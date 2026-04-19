# 06 — Bug Catalogue

Severity rubric: see `README.md`.

Bug IDs prefixed **L-** are new findings from this examination.
IDs prefixed **B-** reference the companion `wcp-uboone-bdt/docs/examinations/05_bugs.md`.

---

## HIGH severity

---

### L-01 — Uncontrolled parallel fan-out with no `wait` (pipeline race)

**Symptom:** Downstream steps read partially-written output files from upstream steps
because the driver returned before all background child processes finished. Analysis
results depend on which files happened to be complete when the next stage started.

**Location:** Every Perl driver that uses background `&` processes:
`convert_bdt.pl`, `convert_cv.pl`, `convert_histo.pl`, `merge_det.pl`,
`merge_weight.pl`, `run_det_sys.pl`, `run_mc_stat.pl`.

The synchronisation idiom `$num % 12 == 11` (or % 10 == 9) runs one foreground process
per 12 (10) background processes to avoid spawning thousands of processes simultaneously.
But the driver returns as soon as the loop finishes — the last batch of up to 11
background children are still running.

**Concrete race:**
- `run_xs.pl` calls `convert_histo.pl 1` (line 5), which forks up to 11 background
  `convert_checkout_hist_xs` jobs and then returns.
- `run_xs.pl` immediately calls `run_det_sys.pl` (line 8), which calls `bin/merge_det`
  reading from `hist_rootfiles/*.root` — files that the still-running
  `convert_checkout_hist_xs` jobs may be writing to.

A similar race exists between `run_numi.pl` step 1 and step 2, and between
`run_gof.pl` steps 2-3.

**Fix sketch:** Add `wait;` after every loop that fires background processes in every
driver. For large fan-outs (e.g. `run_mc_stat.pl` with 100 jobs), use a throttled
parallel dispatch:
```perl
while(my $child = wait()) { last if $child == -1; }
```
or use `xargs -P $NJOBS`.

**Cross-reference:** Efficiency finding E-01, E-03.

---

### L-02 — `pot_counting.cxx` dereferences `end()` iterator

**Symptom:** Undefined behaviour (likely crash or corrupt POT total) when a run/subrun
pair from the checkout ROOT file is not found in `pot_bnb.txt` AND the run number is
outside the range [4000, 50000] (e.g. cosmic runs, test runs, or runs from a new
detector configuration).

**Location:** `wcp-uboone-bdt/apps/pot_counting.cxx:67-72`

```cpp
auto it = map_bnb_infos.find(std::make_pair(pot.runNo, pot.subRunNo));
if (it == map_bnb_infos.end() && pot.runNo >= 4000 && pot.runNo <= 50000) {
    std::cout << "Run: " << pot.runNo << " ... not found!" << std::endl;
} else {
    total_bnb_trigger_no += it->second.first * pass_ratio;   // ← dereferences end()
    total_bnb_pot        += it->second.second * pass_ratio;
}
```

When `it == map_bnb_infos.end()` AND `runNo < 4000` or `runNo > 50000`, the condition
is false and the else-branch executes, dereferencing `end()`. Identical pattern at
lines 100-103 for the EXTBNB map.

**Secondary issue:** `argv[2]` is accessed at line 40 (`TString extbnb_file = argv[2]`)
before the argc guard at line 25 ensures only `argc >= 2`. If the user passes only the
BNB file (argc == 2), `argv[2]` is out of bounds (UB).

**Fix sketch:**
```cpp
if (it == map_bnb_infos.end()) {
    if (pot.runNo >= 4000 && pot.runNo <= 50000)
        std::cout << "not found!" << std::endl;
    // else: silently skip (cosmic / test run)
} else {
    total_bnb_trigger_no += it->second.first * pass_ratio;
    total_bnb_pot        += it->second.second * pass_ratio;
}
```
Also guard `argc < 3` for mode-2 invocations.

---

### L-03 — `plot_hist.cxx` TString substring drops last 2 characters of input path

**Symptom:** The covariance file path passed via `-s<path>` is silently truncated by
2 characters before being opened. For example `-sfile_collapsed_covariance_matrix.root`
becomes `file_collapsed_covariance_matrix.ro`, which `TFile::Open` cannot find (zombie
TFile), causing all covariance-related objects to return NULL and subsequent deref to crash.

**Location:** `wcp-uboone-bdt/apps/plot_hist.cxx:71`

```cpp
cov_inputfile = sss(2, sss.Length()-2);
```

`TString::operator()(int start, int length)` takes (start, length). With start=2 and
length = Length()-2, the resulting string has Length()-2 characters starting at index 2,
so it spans indices [2, Length()-1) — dropping the **last 2 characters** of the original
string.

The fix is `sss(2, sss.Length()-2)` → `sss.Remove(0, 2)` or equivalently
`sss(2, sss.Length())` (using the overload that takes end-index, if available) or
simply `std::string(argv[i]).substr(2)`.

In practice the analysis likely works because users pass the `-s` flag with a path
that happens to end in two characters that, when dropped, still resolve to a valid file
(e.g. a symlink without the extension). But it is structurally broken.

**Fix sketch:** Replace `cov_inputfile = sss(2, sss.Length()-2)` with
`cov_inputfile = sss(2, sss.Length())` (second argument is number of chars to take,
so `Length()` takes all remaining).

---

### L-04 — `det_cov_matrix.cxx` bootstrap output key misspelled; consumers fail silently

**Symptom:** Any code that reads the bootstrap covariance matrix with the correct key
`"cov_mat_bootstrapping_N"` gets NULL, silently skipping the contribution. The actual
key written to the file is the misspelled version.

**Location:** `wcp-uboone-bdt/apps/det_cov_matrix.cxx:151`

```cpp
cov_mat_bootstrapping->Write(Form("cov_mat_boostrapping_%d", run));
//                                      ^^ typo: "boostrapping" missing 't'
```

Compare to the variable name `cov_mat_bootstrapping` (correct) and the histogram name
`cov_mat_boostrapping_N` written to disk.

Any consumer calling `file->Get("cov_mat_bootstrapping_N")` (correct spelling) will
receive NULL. The downstream code typically does not null-check ROOT Get() results.

**Fix sketch:** Correct the typo in the `Form()` string. Search for all consumers of
this key (grep for `cov_mat_b.ostrapping`) and verify they use the same spelling.

**Note:** The companion audit labels this differently; check B-17 in the sibling doc
for related hardcoded-key findings.

---

### L-05 — `prune_weightsep24_trees.cxx` throws `std::out_of_range` on missing knobs

**Symptom:** When a reweight ROOT file does not contain a particular GENIE knob
(e.g. because it was produced with a different GENIE tune version), the base
`prune_weightsep24_trees.cxx` crashes with an uncaught `std::out_of_range` exception.

**Location:** `wcp-uboone-bdt/apps/prune_weightsep24_trees.cxx:~193`

```cpp
std::vector<float>& weights = mcweight->at(knob);  // throws if knob not in map
```

The `_partial_trees` variant (added later) correctly guards this:
```cpp
if (mcweight->find(knob) == mcweight->end()) { /* skip */ continue; }
std::vector<float>& weights = mcweight->at(knob);  // safe
```

The base variant has not been updated with this fix.

**Consumers:** `merge_weight.pl` calls `bin/merge_xf` which internally calls the
pruning logic. If the user runs with a sep24 checkout but uses `prune_weightsep24_trees`
instead of `prune_weightsep24_partial_trees`, any MC file missing a knob causes a crash
with no useful error message (the exception is uncaught at main).

**Fix sketch:** Add the `.find()` guard before the `.at()` call, consistent with the
`_partial_trees` variant.

---

### L-06 — GP kernel mutates cached GPPoint coordinates in-place [dup B-02]

**Symptom:** When GP smoothing is enabled, after the first call to `GPKernel::Mag()` the
input GPPoints have their coordinates replaced by their logarithms. Subsequent calls
compute the log of the already-log-transformed value (log-of-log), producing
silently wrong kernel distances and thus wrong smoothed covariance values. The detector
covariance matrix is corrupted without any warning.

**Location:** `wcp-uboone-bdt/src/GPKernel.cxx:51-52`

```cpp
if (logscale[i]) {
    p1x[i] = log(p1x[i]);   // ← mutates the point in-place
    p2x[i] = log(p2x[i]);
}
```

`p1x` and `p2x` are obtained from `p1.X()` and `p2.X()` which return pointers to
the internal coordinate array of the GPPoint — not a copy.

**Fix sketch:** Copy the coordinates before log-transforming:
```cpp
double v1 = logscale[i] ? log(p1x[i]) : p1x[i];
double v2 = logscale[i] ? log(p2x[i]) : p2x[i];
dist += (v1 - v2) * (v1 - v2) / (fPars[i] * fPars[i]);
```

See `wcp-uboone-bdt/docs/examinations/05_bugs.md:B-02` for extended analysis.

---

### L-07 — `Util.cxx` function definition name mismatches header declaration [dup B-22]

**Symptom:** Linker error (`undefined reference to LEEana::MatrixMatrix`) if any
translation unit includes `Util.h` and calls `MatrixMatrix`. The source file defines
the function under the name `MatrixMatirx` (typo), which does not match.

**Location:**
- Declaration: `wcp-uboone-bdt/inc/WCPLEEANA/Util.h:17` — `MatrixMatrix`
- Definition: `wcp-uboone-bdt/src/Util.cxx:29` — `MatrixMatirx`

**Fix sketch:** Rename either the declaration or the definition (choose one consistent
spelling) and update all call sites.

---

## MEDIUM severity

---

### L-08 — `convert_cv.cxx` operator-precedence obscures subrun-failure filter logic

**Symptom:** The subrun-failure filter condition is correct by accident (C++ operator
precedence produces the intended semantics), but the absence of explicit parentheses
makes future edits extremely error-prone. A well-intentioned refactor could silently
change the logic and wrongly exclude or include subruns.

**Location:** `wcp-uboone-bdt/apps/convert_cv.cxx:546-547`

```cpp
if (failed_num > it->second.size() * fail_percentage && failed_num != 1
    || failed_num > it->second.size() * 0.33 && failed_num == 1)
```

`&&` binds tighter than `||`, so this parses as:
```
(A && B) || (C && D)
```
where A = `failed_num > size * pct`, B = `failed_num != 1`,
C = `failed_num > size * 0.33`, D = `failed_num == 1`.

This is semantically: "if (multiple failures AND fail rate > pct) OR (exactly one
failure AND fail rate > 33%)". This is likely the intended logic (single failures only
trigger removal at a stricter threshold), but it must be stated with parentheses.

**Fix sketch:** Add explicit parentheses:
```cpp
if ((failed_num > it->second.size() * fail_percentage && failed_num != 1) ||
    (failed_num > it->second.size() * 0.33 && failed_num == 1))
```

---

### L-09 — `convert_histo.pl` uses numeric `!=` to filter string header line

**Symptom:** The header comment row in `cv_input.txt` (the `#file #type ...` line) is
not correctly filtered and is passed to `bin/convert_checkout_hist` as a file argument.
In practice the sentinel fires before this becomes a problem, but the logic is wrong.

**Location:** `convert_histo.pl:17`

```perl
if ($temp[0] != -1 && $temp[0] != "\#file") {
```

In Perl, `!=` is numeric comparison. `"\#file"` numifies to 0. This check is equivalent
to `$temp[0] != -1 && $temp[0] != 0`. A row with `$temp[0] == "#file"` (the header)
passes the first condition ("`"#file"` is not -1") and also the second ("`"#file"` is
not 0" — wrong, as `"#file" != 0` numifies to `0 != 0` = false, so the header IS
filtered). The logic is accidentally correct because `"#file"` numifies to 0 and
`0 != 0` is false. But this depends on non-obvious Perl string-to-number coercion.

**Fix sketch:** Use string comparison: `if ($temp[0] ne "-1" && $temp[0] ne "#file")`.

---

### L-10 — `cv_input.txt` dead-tail sentinel trap

**Symptom:** A developer editing the bottom of `configurations/cv_input.txt` (to add or
change a sample) has no effect on the analysis. Lines after the first `-1 end` sentinel
(line 16) are never read. The file contains 4 additional blocks (lines 18–52) that
appear to be live but are dead historical configurations.

**Location:** `configurations/cv_input.txt:16`; `convert_histo.pl:13`

**Fix sketch:** Move the active configuration to the top block (it already is) and
either delete the dead tail blocks, or clearly mark them with a prominent comment:
```
# ===== DEAD BLOCKS BELOW — NOT READ BY THE PARSER =====
# These are historical configurations. To activate one,
# move its contents to the block above the first -1 end line.
```

---

### L-11 — `merge_hist.cxx` accumulates `TFile` handles in a loop (FD leak)

**Symptom:** On a run with many input files (e.g. 15 files × 3 run periods = 45),
`merge_hist` opens 45 ROOT files in a loop without closing or deleting them. This
exhausts the process's file descriptor limit (default 1024 on Linux) if the histogram
map is large enough, causing subsequent `TFile::Open` calls to fail silently and
returning zombie TFile objects whose histograms are NULL.

**Location:** `wcp-uboone-bdt/apps/merge_hist.cxx:86`

```cpp
temp_file = new TFile(out_filename);   // ← reassigned each iteration without close
```

The previous `temp_file` pointer is overwritten without calling `Close()` or `delete`.
Histograms loaded from earlier files remain in memory (ROOT keeps them), but the TFile
handle is leaked.

**Fix sketch:** Close and delete the file **after** all histograms from it have been
cloned or the map has been populated:
```cpp
// After processing all histo_infos for this file:
temp_file->Close();
delete temp_file;
```
Or accumulate all histogram names first, then open/read/close per file.

See efficiency finding E-04.

---

### L-12 — `stat_cov_matrix.cxx` hard-codes output path regardless of run argument

**Symptom:** Despite accepting a `-r<run>` argument, `stat_cov_matrix.cxx` always writes
to the hardcoded path `./hist_rootfiles/run_data_stat.root` regardless of the run number.
Running for run=1 and run=3 sequentially overwrites the same output file.

**Location:** `wcp-uboone-bdt/apps/stat_cov_matrix.cxx:45`

```cpp
TString outfile_name = "./hist_rootfiles/run_data_stat.root";  // always the same
```

The commented-out line `// outfile_name = out_filename;` at line ~90 suggests this was
intentional (run-aggregated output), but the `-r` flag then serves no purpose and a
user who passes `-r1` expecting a per-run output is confused.

**Fix sketch:** Either remove the `-r` flag and document that the output is always
aggregated, or activate `outfile_name = Form("./hist_rootfiles/run%d_data_stat.root", run)`.

**Identical pattern** exists in `stat_pred_cov_matrix.cxx:52`.

---

### L-13 — `det_cov_matrix.cxx` hard-codes 25% floor for zero-prediction diagonal

**Symptom:** For bins where the CV prediction is zero but the detector variation
covariance is nonzero (on the diagonal), the fractional covariance is set to
`1/16 = 0.0625` (25% fractional uncertainty). This is a magic number that may dominate
sparse channels. Bins with zero prediction are typically empty signal-region bins; 
assigning them a 25% detector systematic inflates the total covariance in a
non-physics-motivated way.

**Location:** `wcp-uboone-bdt/apps/det_cov_matrix.cxx:133`

```cpp
(*frac_cov_det_mat)(i,j) = 1./16.;   // 25% uncertainties ...
```

**Fix sketch:** Document the justification for 25%. If there is none, consider using
the average fractional covariance of adjacent non-zero bins (interpolation), or
excluding such bins from the detector covariance entirely (the `add_disabled_ch_name`
mechanism already exists for this).

---

### L-14 — `run_gof.pl` backgrounds `stat_pred_cov_matrix` before `merge_hist` needs it

**Symptom:** `merge_hist -r0 -l0` (line 23 of `run_gof.pl`) reads the output of
`stat_pred_cov_matrix` (line 8). If `stat_pred_cov_matrix` is still running when
`merge_hist` starts, the stat covariance file is incomplete, and `merge_hist` reads
partial or zero data silently.

**Location:** `run_gof.pl:8` (`./bin/stat_pred_cov_matrix -r0 &`) and
`run_gof.pl:20` (`./bin/merge_hist -r0 -l0 -e2 > ...`).

Between lines 8 and 20, `run_det_sys.pl` and `run_xf_sys.pl` are called (lines 11, 17).
These take ~10 min each (per readme). On a fast machine, `stat_pred_cov_matrix` may
finish in time; on a slow machine or under load, it may not.

**Fix sketch:** Remove the `&` from line 8 (run `stat_pred_cov_matrix` synchronously),
or add a `wait` call after line 8 (blocks until it finishes before proceeding).

---

### L-15 — `pot_counting.cxx` accesses `argv[2]` when only `argc >= 2` is guaranteed

**Symptom:** If `pot_counting` is called with only 1 argument (just the BNB file, mode=1
which is the most common case), `argv[2]` is accessed at line 40 before mode is parsed,
reading beyond the end of the argv array (undefined behaviour, typically garbage or crash).

**Location:** `wcp-uboone-bdt/apps/pot_counting.cxx:39-40`

```cpp
TString bnb_file = argv[1];
TString extbnb_file = argv[2];   // ← always read, even in mode=1 (no extbnb needed)
```

The mode is parsed only after these lines (lines 31-37). Even if mode=1 does not
*use* `extbnb_file`, it is unconditionally assigned.

**Fix sketch:** Move the argv assignments after mode parsing, and guard with
`if (argc > 2)` for `extbnb_file`.

---

## LOW severity

---

### L-16 — `merge_numi_flux_geom.pl` hard-codes host-specific paths

**Symptom:** Script silently fails (non-zero exit, unchecked) on any machine other
than the author's. All four `system()` calls reference absolute paths under
`/data1/xqian/MicroBooNE/` and `/home/xqian/wire-cell/`.

**Location:** `merge_numi_flux_geom.pl:3-10`

**Fix sketch:** Replace hard-coded paths with variables or config-file entries.
At minimum, add `die "..." if $? != 0;` after each `system()` call.

---

### L-17 — `summarize_pot.pl` hard-codes host-specific paths

**Symptom:** Same issue. References `/data0/xqian/MicroBooNE/run4a_files/...`.

**Location:** `summarize_pot.pl:4-6`

---

### L-18 — `check_xf.pl` uses wrong column index for `xf_input.txt`

**Symptom:** `bin/check_xf_weight_xs` is invoked with `$temp[2]` (the weight-index
integer, column 2) instead of `$temp[3]` (the weight ROOT-file path, column 3). The
binary receives a numeric string as its input file path and fails with a file-not-found
error. The check never actually validates anything.

**Location:** `check_xf.pl:6`

```perl
system("./bin/check_xf_weight_xs $temp[2]");
#                                       ^^ should be $temp[3]
```

**Fix sketch:** Change `$temp[2]` to `$temp[3]`.

---

### L-19 — `file_ch.txt` vs `xf_file_ch.txt` use different sentinel case

**Symptom:** If a parser assumes the same sentinel for both files (e.g. copies the same
parsing function), one file will not terminate correctly. Not currently a bug in the
compiled code (which reads them in different contexts), but is a maintenance hazard.

**Locations:**
- `configurations/file_ch.txt:46` — sentinel `end end`
- `configurations/xf_file_ch.txt` — sentinel `End End`

---

### L-20 — `run_det_sys.pl` oscillation mode runs all 10 sources including WireModdEdx

**Symptom:** In default mode, source i=5 (WireModdEdx) is explicitly skipped.
In oscillation mode (any ARGV), all 10 sources run including i=5. This inconsistency
is undocumented. If WireModdEdx was skipped for a physics reason (low statistics, bad
MC), enabling it in oscillation mode introduces an unreliable covariance component.

**Location:** `run_det_sys.pl:5-15`

```perl
if ($num1 == 0) {
    if ($i == 5) { print "Skip WireMod dEdx\n"; }
    else { system("./bin/det_cov_matrix -r$i &"); }
} else {
    print "Oscillation! \n";
    system("./bin/det_cov_matrix -r$i -o1 &");   # no skip of i=5
}
```

**Fix sketch:** Document why i=5 is skipped and whether the reason applies to
oscillation analyses. If it does, add the same `if ($i == 5)` guard in the oscillation
branch.

---

### L-21 — `prune_weight` family has three copy-paste variants that have diverged

**Symptom:** Bugs fixed in one variant of the prune-weight code do not flow to the
others. Users may silently use the wrong variant for their production epoch.

**Locations:**
- `prune_weightmar18_trees.cxx` — pre-Sep-2024 knob list; lacks Geant4 reinteraction knobs.
- `prune_weightsep24_trees.cxx` — adds Geant4 knobs; missing `.find()` guard (see L-05).
- `prune_weightsep24_partial_trees.cxx` — adds `.find()` guard; correct for partial-weight files.
- `prune_weightsep24_trees_numi.cxx` — adds "pad to 1000 universes" block for NuMI; missing from others.

**Fix sketch:** Create a single parametrised implementation that selects knob lists and
padding behaviour from command-line flags or a config file. At minimum, backport the
`.find()` guard to all three sep24 variants.

---

### L-22 — `WienerSVD_3D.C` contains debug print statements

**Symptom:** Running `WienerSVD_3D.C` floods stdout with `"here 1"`, `"here 2"`, etc.
debug messages that were never removed after development.

**Location:** `wcp-uboone-bdt/src/WienerSVD_3D.C:284, 301, 303, 308, 312-325`

---

## STYLE

---

### L-23 — `applyNuMIGeomtryWeights` binary name contains a typo

The source file, binary name, and all callers use `Geomtry` instead of `Geometry`.
The usage string inside the binary prints the correct spelling.

**Locations:**
- `wcp-uboone-bdt/apps/applyNuMIGeomtryWeights.cxx` (file name)
- `merge_numi_flux_geom.pl:3,6,8,10` (callers)

---

### L-24 — `readme` line 26 misspells binary name `pot_countng_mc`

**Location:** `readme:26` — `./bin/pot_countng_mc` should be `./bin/pot_counting_mc`.

---

### L-25 — `readme` line 32 uses singular `./configuration/` (missing 's')

**Location:** `readme:32` — `./configuration/cv_input.txt` should be
`./configurations/cv_input.txt`. Will mislead new users looking for the config directory.

---

### L-26 — `run_xs.pl` comment says "1→16" but the loop runs 1–17

**Location:** `run_xs.pl:10` — comment says "GEANT4 1-->16"; `run_xf_sys.pl` loops
`$i=1; $i<18` (1 through 17 inclusive). The 17th knob (`UBGenieFluxSmallUni`) is a
genuine systematic, not GEANT4. The comment is misleading on both counts.

---

### L-27 — `TLee` public API contains two misspellings of Feldman-Cousins [dup B-23]

**Location:** `wcp-uboone-bdt/inc/WCPLEEANA/TLee.h`
- Line ~149: `Exe_Fledman_Cousins_Asimov` (should be `Feldman`)
- Line ~155: `Exe_Fiedman_Cousins_Data` (should be `Feldman`)

These are baked into the public API and into any call sites in `read_TLee_v20.cxx`.
Renaming requires updating all callers.

---

## Summary table

| ID | Severity | File:line | One-line description |
|----|----------|-----------|---------------------|
| L-01 | High | `*.pl` drivers | Fan-out with no `wait` → pipeline race |
| L-02 | High | `pot_counting.cxx:67-72` | Dereference of `end()` iterator |
| L-03 | High | `plot_hist.cxx:71` | TString substring truncates last 2 chars of path |
| L-04 | High | `det_cov_matrix.cxx:151` | `"boostrapping"` typo in output ROOT key |
| L-05 | High | `prune_weightsep24_trees.cxx:~193` | Unguarded `at()` throws on missing knob |
| L-06 | High | `GPKernel.cxx:51-52` | In-place mutation of GPPoint corrupts repeated calls |
| L-07 | High | `Util.h:17` / `Util.cxx:29` | `MatrixMatrix` vs `MatrixMatirx` → linker error |
| L-08 | Medium | `convert_cv.cxx:546-547` | Missing parens around operator-precedence logic |
| L-09 | Medium | `convert_histo.pl:17` | Numeric `!=` on string silently wrong |
| L-10 | Medium | `cv_input.txt:16` | Dead tail blocks after sentinel are invisible trap |
| L-11 | Medium | `merge_hist.cxx:86` | TFile FD leak in loop — exhausts FD limit |
| L-12 | Medium | `stat_cov_matrix.cxx:45` | Hard-coded output path ignores `-r` argument |
| L-13 | Medium | `det_cov_matrix.cxx:133` | Hard-coded 25% floor for zero-prediction bins |
| L-14 | Medium | `run_gof.pl:8` | `stat_pred_cov_matrix` backgrounded before `merge_hist` needs it |
| L-15 | Medium | `pot_counting.cxx:40` | `argv[2]` accessed before argc guard |
| L-16 | Low | `merge_numi_flux_geom.pl:3-10` | Host-specific hard-coded paths |
| L-17 | Low | `summarize_pot.pl:4-6` | Host-specific hard-coded paths |
| L-18 | Low | `check_xf.pl:6` | Wrong column index; binary gets integer not path |
| L-19 | Low | `file_ch.txt:46` / `xf_file_ch.txt` | Terminator case mismatch |
| L-20 | Low | `run_det_sys.pl:5-15` | Oscillation mode silently enables skipped source i=5 |
| L-21 | Low | `prune_weight*.cxx` | Three copy-paste variants diverged; bugs not backported |
| L-22 | Low | `WienerSVD_3D.C:284+` | Debug `"here N"` print statements left in |
| L-23 | Style | `applyNuMIGeomtryWeights.cxx` | Typo in binary name propagated to all callers |
| L-24 | Style | `readme:26` | `pot_countng_mc` should be `pot_counting_mc` |
| L-25 | Style | `readme:32` | `configuration/` should be `configurations/` |
| L-26 | Style | `run_xs.pl:10` | Comment says "1→16" but loop runs 1→17 |
| L-27 | Style | `TLee.h:~149,155` | `Fledman`/`Fiedman` typos in public API |
