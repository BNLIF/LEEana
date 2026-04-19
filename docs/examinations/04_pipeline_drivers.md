# 04 — Pipeline Drivers Reference

All Perl drivers live at the LEEana repo root.  They assume the working directory is
the LEEana root and that `./bin/` is populated by building `wcp-uboone-bdt`.
ROOT macros assume `root` is on PATH and that ROOT files referenced exist at relative
paths from the LEEana root (or from within `wiener_svd/`).

---

## Perl drivers

### `convert_bdt.pl` (45 lines)

Recomputes BDT scores for all samples in `filelist_bdt`.

**Input file:** `filelist_bdt` — whitespace-separated table; relevant columns:
- col 0+1: input path prefix + filename (concatenated, no separator).
- col 2+1: output path prefix + filename.
- col 4: BDT mode (1=standard, 2=genie-run, 3=NuMI, else=default).
- col 5: period/run suffix used in mode 2.

Terminator: first word == `"end"`.

**Binary invoked:** `bin/bdt_convert` with one of four argument patterns:
- Mode 1: `-l./training_list/list.dat`
- Mode 2: `-g$temp[5] -l./training_list/list.dat`
- Mode 3: `-s1`
- Default: no extra flags

**Parallelism:** Every 12th entry (index % 12 == 11) runs synchronously (no `&`);
the rest are forked as background shell processes. No `wait` at end.
See bugs L-01.

**Fragile points:**
- `open(infile,"filelist_bdt")` — no `or die`; silently empty loop if file is missing.
- Filenames are interpolated unquoted into the shell string (`system("bin/bdt_convert $temp[0]$temp[1] ...")`);
  paths with spaces or shell metacharacters will break.
- All `system()` return values are unchecked; silent failure on any non-zero exit.

---

### `convert_cv.pl` (22 lines)

Deduplicates CV rootfiles and removes failed subruns.

**Input file:** `filelist_cv` — columns: 2=input path, 3=output path, 1=common suffix.
Terminator: first word == `"end"`.

**Binary:** `bin/convert_cv_spec` with `$temp[2]$temp[1]` (input) and `$temp[3]$temp[1]` (output).

**Parallelism:** index % 12 == 11 runs synchronously; rest forked. No `wait`.

**Fragile points:** Same `open` without `or die`; same unquoted interpolation;
same unchecked `system()`.

---

### `convert_histo.pl` (39 lines)

Central-value histogram production.

**Input:** `./configurations/cv_input.txt` — path hard-coded (cannot use a per-analysis
subdir without editing the script). Reads until the first `-1` sentinel.

**ARGV[0]** selects the binary:
- 0 (default): `./bin/convert_checkout_hist`
- 1: `./bin/convert_checkout_hist_xs`
- 2: `./bin/convert_checkout_hist -o1`

**Key parsing bug (line 13):** `if ($temp[0] != "\#file")` uses the numeric `!=`
operator to compare against the string `"#file"`. Since `"#file"` numifies to 0, this
condition is `0 != 0` = false — the header comment line is **not** filtered out and
will be passed to `bin/convert_checkout_hist` as a file argument, causing a silent error
(the binary receives a mangled path and opens a non-existent file). In practice this
only works because the sentinel `$temp[0] == -1` (numeric) fires first when the code
hits the actual `-1` line; the header comment row has a non-numeric first field so
`$temp[0]` numifies to 0 and the `!= -1` check (line 13 first branch) passes accidentally.
See bug L-09.

**Parallelism:** index % 12 == 11 runs synchronously; rest forked. No `wait`. The binary
reads the same `cov_input.txt` and `file_ch.txt` concurrently from all forked processes —
these are read-only, so no file corruption, but it is 12× concurrent disk I/O.

---

### `merge_det.pl` (18 lines)

Merges CV + detector-variation checkouts for bootstrap.

**Input:** `./configurations/det_file.txt` — **this file is not committed**; it must
be staged manually from the appropriate `configurations/<analysis>/det_file.txt`.

**Binary:** `./bin/merge_det $filename` — passes the entire input line as the argument
string; `merge_det` splits it. The `$filename` is the raw line from the file including
trailing whitespace from the chomp'd split, which may contain invisible spaces.

**Parallelism:** index % 10 == 9 runs synchronously. No `wait`.

---

### `merge_weight.pl` (20 lines)

Merges CV + reweight checkouts for each of the 17 systematic knobs.

**Input:** `./configurations/xf_file.txt` — **not committed**; must be staged.
Terminator: first word == `'end'` (single-quoted; different style from other drivers).

**Binary:** `./bin/merge_xf $filename`.

**Parallelism:** index % 12 == 11 runs synchronously. No `wait`. With 17 knobs × 9
files each = 153 rows, this spawns up to ~140 background processes simultaneously.

---

### `merge_numi_flux_geom.pl` (11 lines)

Applies NuMI geometry reweighting to intrinsic-nue and nu-overlay NuMI samples.

**Paths:** Fully hard-coded to `/data1/xqian/MicroBooNE/...` and
`/home/xqian/wire-cell/wcp-uboone-bdt/scripts/NuMI_Geometry_Weights_Histograms.root`.
Will silently fail (non-zero exit, unchecked) on any other host.

**Binary:** `./bin/applyNuMIGeomtryWeights` — note the typo (`Geomtry` not `Geometry`).
The binary name is also a typo (the `.cxx` file is identically named). The usage string
inside the binary prints the correct spelling, which will mislead users copying it.

Fires 4 background jobs (`&`) with no `wait`. A downstream script that reads the output
files (e.g. `merge_xf`) may race with these jobs. See bug L-23 (typo) and L-16 (paths).

---

### `run_det_sys.pl` (16 lines)

Runs detector covariance matrix calculation for each of 9-10 sources.

**Loop:** `$i = 1..10`.
- Default (no ARGV): skips i=5 (prints "Skip WireMod dEdx") — only **9** of 10 sources
  run. The `readme` states "10 sources" without mentioning this skip. See bug L-20.
- With any ARGV: switches to oscillation mode (`-o1`) and runs **all 10** including i=5.
  This inconsistency between default and oscillation mode is undocumented.

**Binary:** `./bin/det_cov_matrix -r$i` or `-r$i -o1`.

**Parallelism:** All 9-10 jobs are fired with `&` simultaneously (no modulo throttle).
No `wait`. These jobs each read the merged det ROOT files and write individual output
ROOT files; they are independent, so the race is only against any downstream steps.

---

### `run_xf_sys.pl` (20 lines)

Runs flux/XS/GENIE covariance matrix calculation for 17 knobs.

**Loop:** `$i = 1..<18` (i.e. 1 through 17 inclusive).
**Synchronisation:** `$i % 9 == 8` — only i=8 and i=17 run foreground. All others
are forked. No `wait` at end.

**Binary:** `./bin/xf_cov_matrix -r$i` or `-r$i -o1`.

The comment in `run_xs.pl:10` says "GEANT4 1-->16" (16 knobs), but the actual loop
here runs 1–17. See bug L-26.

---

### `run_mc_stat.pl` (11 lines)

Runs 100 iterations of `bin/merge_hist` at different LEE strengths to populate the
Bayesian MC-stat log files.

**Loop:** `$i = 0, 1, ..., 99`; LEE strength = 0.03 × i → 0, 0.03, 0.06, ..., 2.97.
**Synchronisation:** i % 12 == 11 — one foreground run per 12. Final batch (i=88..99)
all fires as background with no `wait`. On typical hardware, spawns ~88 simultaneous
`bin/merge_hist` processes in the final sequence, massively oversubscribing CPUs.

**Output:** `./mc_stat/$i.log` — the directory `mc_stat/` must exist (not created by the
script; not guaranteed to exist after a fresh clone).

See efficiency finding E-03.

---

### `run_xs.pl` (20 lines)

Orchestrates the full cross-section analysis pipeline **sequentially**:

1. `./convert_histo.pl 1` — CV xs histograms.
2. `./run_det_sys.pl` — detector covariance.
3. `./run_xf_sys.pl` — flux/xs covariance.
4. `./bin/xs_cov_matrix -r17` — POT/target xs covariance (hardcoded r=17).
5. `./bin/merge_hist -r0 -l0 -e2 > ./mc_stat/xs_tot.log` — MC stat.
6. `./bin/merge_hist_xs -r0 -l0` — final merged xs histograms.

Steps 2 and 3 are independent after step 1 finishes, but are run **serially**.
Each `system()` return value is unchecked — if step 1 fails, steps 2-6 run on stale
inputs without warning. See efficiency finding E-05 and bug L-XX.

---

### `run_numi.pl` (14 lines)

Orchestrates NuMI analysis pipeline sequentially:

1. `./convert_histo.pl` — CV histograms.
2. `./run_xf_sys.pl` — flux/xs covariance (no det sys for NuMI by default).
3. `./bin/merge_hist -r0 -l0 -e2 > ./mc_stat/xs_tot.log` — MC stat.
4. `./bin/merge_hist -r0 -l1` — final merge with LEE strength 1.

---

### `run_gof.pl` (23 lines)

Orchestrates the GoF analysis pipeline:

1. `./convert_histo.pl` — CV histograms.
2. `./bin/stat_pred_cov_matrix -r0 &` — MC pred stat **backgrounded**.
3. `./run_det_sys.pl` — detector covariance.
4. `./run_xf_sys.pl` — flux/xs covariance.
5. `./bin/merge_hist -r0 -l0 -e2 > ./mc_stat/0.log` — MC stat.
6. `./bin/merge_hist -r0 -l0` — final merge.

**Race condition (bug L-14):** Step 2 is backgrounded. Step 3 (`run_det_sys.pl`) then
calls `bin/det_cov_matrix` which reads from `hist_rootfiles/` — the same directory
that `stat_pred_cov_matrix` writes to. On a fast filesystem this is likely harmless
(they write different files), but step 5 (`merge_hist`) reads the stat file produced
by step 2; if step 2 is still running when step 5 starts, the stat file is incomplete.

---

### `summarize_pot.pl` (6 lines)

Builds `pot_bnb.txt` and `pot_extbnb.txt` from database POT files.

Two `system("cat ... | grep 0 | awk ...")` shell pipelines. Hard-coded paths:
- `./pot_counting/data_bnb_mcc9.1_wcp*.txt`
- `/data0/xqian/MicroBooNE/run4a_files/prod_bnb_optfilter_mcc9.0_reco1_H_potdb.txt`

See bug L-17.

---

### `gen_training_list.pl` (13 lines)

Generates BDT training/test event lists.

Hard-coded input paths: `./old_files/run{1,3}_{intrinsic_nue,bnb_nu,...}_POT*.root`.
The `old_files/` directory is **not in git**. Will fail silently if it doesn't exist.

---

### `check_failure.pl` (9 lines)

Runs `bin/check_failures` on each file listed in `filelist` (column 1, 0-indexed).
`filelist` is not the same as `filelist_bdt` or `filelist_cv`; it must be pre-staged.
No `or die` on `open`. No `wait` (fires jobs sequentially via blocking `system()`).

---

### `check_xf.pl` (7 lines)

Runs `bin/check_xf_weight_xs` on the weight ROOT file for each entry in
`./configurations/xf_file.txt`.

Uses `$temp[2]` as the path. However `xf_file.txt` follows the `xf_input.txt` schema
(4+ columns: file-code, type, weight-index, path, …), making `$temp[2]` the **weight
index** (an integer), not the path (`$temp[3]`). This means the binary receives a
numeric string as its input filename, guaranteed to fail. See bug L-18.

---

## wiener_svd/ macros

### `copy.pl` (6 lines)

Stages files from the LEEana root into the `wiener_svd/` working directory:

```
cp ../mc_stat/xs_tot.log   ./mcstat/
cp ../hist_rootfiles/DetVar/cov*.root   ./DetVar/
cp ../hist_rootfiles/XsFlux/cov_*.root  ./XsFlux/
cp ../merge_xs.root  .
```

No `mkdir -p` — the destination subdirs (`./mcstat/`, `./DetVar/`, `./XsFlux/`) must
pre-exist. No error checking on `cp`.

### `convert_wiener_simple.C`

Reads `merge_xs.root` (produced by `bin/merge_hist_xs`) and assembles the inputs for
the Wiener-SVD call: data minus background, response matrix (collapsed), total
covariance (stat + mcstat + det + xf + add_cov). Calls `WienerSVD()`. Writes `wiener.root`.

Key construction (line 28):
```
nbin_meas = hdata_obsch_1->GetNbinsX() + 1 + hdata_obsch_2->GetNbinsX() + 1
```
The `+1` includes the overflow bin — intentional, to capture events above the histogram
upper edge. If the analysis later changes the upper bin boundary, this is consistent
automatically.

CNP stat variance computed inline (lines 72-90) mirroring the formula in `TLee.cxx`
but as a standalone block without a shared helper function.

### `convert_wiener_simple_rw.C`

Variant of `convert_wiener_simple.C` that applies an additional reweighting step
before Wiener-SVD. Structurally identical; diverges from the base by a post-read
reweighting block.

### `convert_wiener.C`

More complex variant with many boolean flags at the top of the file:
`just_stat_uncertainty`, `use_fakedata`, `new_noise`, `add_noise`, `data_wgu`,
`systematics_wgu`. These are set by editing the source file. Easy to commit in the
wrong state without noticing.

### `plot.C`, `plot1.C`

Plot unfolded cross-section vs MC truth and theory predictions from `output.root`
(produced by `bin/wiener_example`). Reference external files via relative paths from
`wiener_svd/`.

### `plot_xs_tot.C`, `plot_xs_tot_perE.C`

Publication-quality total xs plots. Use fixed axis ranges and colour schemes. No known
bugs.

### `plot_xs_{muon,hadron}.C`, `plot_xs_ratio_{muon,hadron}.C`

Differential xs plots and ratio plots.

### `calc_bin_center.C`

Computes bin centres from a `TH1` binning definition. Simple utility; no bugs.

### `plot_correlation.C`

Plots the correlation matrix of the unfolded covariance. Reads `output.root`.

---

## Top-level ROOT macros

| Macro | Purpose |
|-------|---------|
| `DrawOption.cc` | Global ROOT style (`snStyle`). Loaded at runtime via `gROOT->ProcessLine(".x DrawOption.cc")` in `plot_hist.cxx` — re-parsed by Cling each invocation. |
| `compare_merge.C` | Overlay `histo_1`/`histo_3` from `merge_r1.root` vs `merge_r3.root`, POT-scaled. Simple visual cross-check of two analysis configurations. |
| `compare_nueCC.C` | Overlay CV nueCC histogram against DetVar (LYDown) and XsFlux (cov_1) predictions for a single channel. |
| `compare_r13.C` | Run 1 vs Run 3 numuCC/nueCC overlay from `merge.root` outputs (includes a commented block that reads directly from `T_eval` friend trees). |
| `compare_weights.C` | Overlay new vs old weight reconstructed-Enu histograms for cross-checking GENIE tune changes. |
| `test_fake7.C` | Hard-coded TGraphAsymmErrors + sine-fit test for fake dataset 7. Not production code. |
| `test_fid_boundary.C` | Scans `truth_vtxX/Y/Z` min/max and checks nue_score acceptance ratios. Quick fiducial sanity check. |
| `test_xs.C` | Integrates `truth_isCC`-weighted numuCC signal for Enu bins with `weight_cv` error propagation. Quick xs sanity check. |

---

## plot_script/ deep-dive macros

### `plot_check_det.C`

Loads up to 9 detector covariance files, computes fractional covariance diagonals,
overlays them per observation channel.

**Bug:** Allocates `new TFile*[9]` but the loop index for initialisation goes to 9;
`file[5]` is typically skipped (matching the `run_det_sys.pl` i=5 skip). If the loop
iterates over all 9 elements unconditionally, any uninitialised pointer is dereferenced.
In practice, the loop is `for(i=0; i<n_src; i++)` where `n_src` is read from a config
or hardcoded. Verify before use.

### `plot_check_xf.C`

Loads up to 17 covariance files via `new TFile(Form("hist_rootfiles/XsFlux/cov_%d.root",i+1))`.
If any upstream `xf_cov_matrix` job failed, `cov_*.root` doesn't exist; `TFile::Open`
returns a non-null zombie; `Get("frac_cov_xf_mat_N")` returns NULL; dereference → segfault.

### `plot_check_stat.C`, `plot_check_gof.C`

Read `merge.root` + `mc_stat/*.log` + `run_pred_stat.root`. No unusual bugs; assume
those files are fully written.

### `plot_check_nueCCFC.C`, `plot_check_nueCCFC_Det.C`

Overlay nueCCFC signal vs background by component. The `_Det` variant additionally
overlays detector variation shapes.

### `plot_check_numuCCFC.C`, `plot_check_numuCCFC_Det.C`, `plot_check_numuCCFC_xf.C`

Same structure for numuCC FC channel. The xf variant overlays flux/xs variation shapes.

### `plot_check_xs.C`

Cross-section validation overlay (predicted vs data background-subtracted).

### `plot_check_breakdown.C`

Stacked histogram of the signal/background breakdown by component, driven by
`bin/plot_hist -b1` output.
