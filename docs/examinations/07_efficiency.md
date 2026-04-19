# 07 — Efficiency and Performance Findings

This document covers resource-use concerns: I/O patterns, CPU overhead, parallelism,
and memory. Findings are tagged with whether they affect the **orchestration layer**
(LEEana Perl drivers and ROOT macros) or the **C++ layer** (wcp-uboone-bdt apps/src).

Cross-references to the companion examination:
`wcp-uboone-bdt/docs/examinations/06_efficiency.md` — contains additional findings
on the C++ library layer.

---

## E-01 — Minuit FCN rebuilds and inverts full covariance on every evaluation

**Layer:** C++ (`TLee.cxx`)
**Impact:** Severe — repeated O(N³) matrix inversion inside the optimisation loop.

`Minimization_Lee_strength_FullCov` passes a lambda FCN to Minuit2. Inside the FCN,
for each function evaluation the CNP stat covariance is rebuilt from scratch
(`TLee.cxx:232-278`), added to the full systematic covariance, and the combined matrix
is inverted (`TLee.cxx:262`).

The systematic covariance (det + xf + xs + add_cov) does not depend on the LEE
strength α and could be precomputed once. Only the CNP stat diagonal depends on α
through N_pred(i; α). The current structure pays the O(N³) inversion cost on every
Minuit call (typically hundreds of evaluations per fit).

**Estimate:** For N = 100 channels, a matrix inversion is ~10⁶ FLOPs. At 500
evaluations per fit, this is ~5×10⁸ FLOPs for the inversion alone, which on a single
core takes ~0.5 ms — fast, but the repeated rebuild of the full covariance object
(ROOT matrix operations) and the Python-style closure captures may add latency.
For N = 500+ (multi-channel analyses), the O(N³) cost becomes dominant.

**Fix sketch:** Cache the systematic covariance component. Only recompute the CNP
stat diagonal (O(N)) at each FCN call and use `Sherman-Morrison` updates or a
pre-inverted system matrix.

---

## E-02 — `run_mc_stat.pl` spawns 100 concurrent `merge_hist` processes

**Layer:** Orchestration (`run_mc_stat.pl`)
**Impact:** High — massively oversubscribes CPUs; I/O contention on the histogram files.

`run_mc_stat.pl` loops from i=0 to 99, firing `./bin/merge_hist -r0 -e2 -l$lee_strength`
for 100 LEE strength values. With synchronisation at every 12th iteration (i%12==11),
the final 88 processes are all backgrounded before the loop exits — no `wait` at end.

On a 16-core machine, this is ~5× CPU oversubscription. Each `merge_hist` instance
reads the same set of histogram ROOT files (from `hist_rootfiles/`) and the same
covariance files concurrently, producing heavy random-access I/O contention.

More fundamentally: `merge_hist` reads all input files and recomputes the full
merged histogram from scratch for each LEE strength. The only thing that changes
between invocations is the LEE-signal scale applied to the intrinsic-nue channels.
A single `merge_hist` run that sweeps LEE strengths internally would reduce I/O
by ~100×.

**Fix sketch:** Add a `-l_sweep start:step:stop` option to `merge_hist` so all 100
LEE strengths are processed in one I/O pass. Alternatively, serialise the 100 runs and
accept the sequential cost (~5 min × 100 = 8h → replace with one 5-min run).

---

## E-03 — `run_xs.pl` runs det_sys and xf_sys serially despite independence

**Layer:** Orchestration (`run_xs.pl`)
**Impact:** Medium — wastes ~10 min of wall-clock time per analysis run.

After `convert_histo.pl` finishes (step 1), `run_det_sys.pl` and `run_xf_sys.pl` are
called sequentially (lines 8 and 11 of `run_xs.pl`). Both read from
`hist_rootfiles/*.root` (written by step 1) and write to separate subdirectories
(`DetVar/` and `XsFlux/`). They are fully independent.

The readme estimates `run_det_sys.pl` takes ~10 min. Running them in parallel would
halve wall time.

**Fix sketch:**
```perl
system("./run_det_sys.pl &");
system("./run_xf_sys.pl &");
wait();
```

---

## E-04 — `merge_hist.cxx` opens all histogram ROOT files simultaneously (FD exhaustion)

**Layer:** C++ (`merge_hist.cxx`)
**Impact:** Medium — silently fails with zombie TFiles once FD limit (~1024) is reached.

The outer loop in `merge_hist.cxx:80-113` opens `new TFile(out_filename)` for each
entry in `map_inputfile_info` without closing the previous file. With 15 MC/data files
× 3 run periods = 45 open TFiles, each typically referencing dozens of histogram objects,
the total FDs consumed can approach system limits in analyses with many channels.

**Fix sketch:** After loading all histograms from a given TFile into `map_name_histogram`,
close and delete the TFile:
```cpp
for (size_t i = 0; i != all_histo_infos.size(); i++) {
    htemp = (TH1F*)temp_file->Get(std::get<0>(all_histo_infos.at(i)));
    if (htemp) {
        map_name_histogram[...] = std::make_pair((TH1F*)htemp->Clone(), pot);
    }
}
temp_file->Close();
delete temp_file;
```
Cloning the histograms detaches them from the TFile, allowing safe closure.

---

## E-05 — Multiple full traversals of the same ROOT TTree in `merge_det.cxx`

**Layer:** C++ (`merge_det.cxx`)
**Impact:** Medium — 3× event loop over a potentially large TTree.

`merge_det.cxx` performs three separate passes over `T_eval_cv` and `T_eval_det`:
1. First pass (~line 909): build `map_rs_re_cv` (run/subrun → event list).
2. Second pass (~line 931): build `map_rs_re_det`.
3. Third pass (~line 987): read the full event content for matched events and write output.

A single pass that builds the map while buffering matched events would eliminate
two of the three reads. For large detector-variation samples (O(10M events)), this
could save tens of minutes.

**Fix sketch:** Build the event-key sets on the first pass and buffer the data into
memory (or an indexed TChain), then write in a single sequential pass.

---

## E-06 — `SetBranchStatus("*",1)` silently re-enables all branches after selective disable

**Layer:** C++ (`merge_det.cxx:814`, `merge_xf.cxx:647`)
**Impact:** Medium — undoes per-branch optimisation, causing reads of all branches per
event (much more I/O than necessary for large trees).

The correct pattern for selective branch activation is:
```cpp
tree->SetBranchStatus("*", 0);   // disable all
tree->SetBranchStatus("run", 1); // enable only what's needed
```

In `merge_det.cxx:814`, `T_eval_det->SetBranchStatus("*",1)` is called before a block
of per-branch `SetBranchStatus("branch",1)` calls. The `"*",1` makes everything active;
subsequent per-branch calls are no-ops. The overhead is proportional to the number of
branches in `T_eval_det` (hundreds in `eval.h`/`tagger.h`).

---

## E-07 — Per-event tuple string comparison in histogram-fill inner loop

**Layer:** C++ (`convert_checkout_hist.cxx:307-341`)
**Impact:** Low-to-medium depending on event count.

Inside the per-event loop, `get_kine_var()`, `get_cut_pass()`, and `get_weight()` are
called for each (channel, add_cut, weight) triple via string comparison (matching
channel names and cut names as `TString`). For 25 channels and 10 million events:
- 25 × 10⁷ = 2.5 × 10⁸ string comparisons per run.

**Fix sketch:** Pre-resolve the (channel, add_cut, weight) combinations to integer
indices at startup, and use switch/indexed dispatch in the inner loop.

---

## E-08 — `bayes.cxx` sets `TF1::Npx=60000` per component (excessive FFT resolution)

**Layer:** C++ (`bayes.cxx`)
**Impact:** Low — unnecessary CPU per bin.

`add_meas_component()` creates a `TF1` with `Npx=60000` points for each per-event
weight component. The posterior is computed by FFT convolution. For typical analyses
with ~100 events per bin, 60,000-point FFTs provide far more resolution than is
needed (the Poisson width at N=100 is σ≈10, which needs only ~1000 points to resolve
at 0.01-event precision).

**Estimate:** For a 10-bin histogram with 100 events/bin and 100 components/bin:
100 × 10 × 60,000 point operations at each convolution step = 6×10⁷ per histogram
call. With ~45 files × ~25 channels = 1125 calls, this is ~7×10¹⁰ FFT operations.

**Fix sketch:** Reduce `Npx` to 5000 or adapt it to the expected Poisson width:
```cpp
int Npx = std::max(1000, 100 * (int)sqrt(weight_sum));
```

---

## E-09 — `run_mc_stat.pl` requires `mc_stat/` directory to pre-exist

**Layer:** Orchestration (`run_mc_stat.pl:6`)
**Impact:** Low — silent failure if the directory does not exist.

The redirection `> ./mc_stat/$i.log` silently fails if `./mc_stat/` does not exist
(shell creates the file in the current directory, or the redirection simply fails).
There is no `mkdir -p mc_stat` in the script or in the readme setup instructions.

**Fix sketch:** Add `mkdir -p mc_stat` at the top of `run_mc_stat.pl`.

---

## E-10 — `summarize_pot.pl` reads the same POT database files twice

**Layer:** Orchestration (`summarize_pot.pl:4-6`)
**Impact:** Negligible — but wasteful.

Two `cat ... | grep 0 | awk` pipelines read overlapping sets of POT database files
(the EXT pipeline reads additional files beyond the BNB pipeline). A single pass
producing both `pot_bnb.txt` and `pot_extbnb.txt` simultaneously would be cleaner.

---

## E-11 — Serial Feldman-Cousins toy loop; no ROOT IMT parallelism

**Layer:** C++ (`read_TLee_v20.cxx:834`)
**Impact:** Medium — 500 toys × 1 full fit each = potentially hours for complex analyses.

The FC toy loop is serial. ROOT supports implicit multi-threading (IMT) via
`ROOT::EnableImplicitMT()`, and Minuit2 is thread-safe. The toy pseudo-experiments
are independent (each with different random variations) and could be trivially
parallelised.

**Fix sketch:** Parallelise over toys using `ROOT::TThreadExecutor` or a simple
`std::thread` pool. Each toy needs its own `TLee` instance (they must not share state).

---

## Summary

| ID | Layer | Impact | Description |
|----|-------|--------|-------------|
| E-01 | C++ | Severe | Minuit FCN rebuilds + inverts full covariance every call |
| E-02 | Orchestration | High | 100 concurrent `merge_hist` processes; 100× redundant I/O |
| E-03 | Orchestration | Medium | `det_sys` and `xf_sys` run serially; ~10 min wasted per run |
| E-04 | C++ | Medium | All TFiles kept open simultaneously in `merge_hist.cxx` |
| E-05 | C++ | Medium | 3 event-loop passes over the same TTree in `merge_det.cxx` |
| E-06 | C++ | Medium | `SetBranchStatus("*",1)` undoes selective branch disable |
| E-07 | C++ | Low-Med | Per-event string dispatch in histogram-fill inner loop |
| E-08 | C++ | Low | 60,000-point FFT per component in `bayes.cxx` |
| E-09 | Orchestration | Low | `mc_stat/` directory must pre-exist (silent failure) |
| E-10 | Orchestration | Negligible | POT files read twice by `summarize_pot.pl` |
| E-11 | C++ | Medium | Feldman-Cousins toy loop is single-threaded |
