# 05 — Configuration File Schemas

All configuration files live under `configurations/`.  The driver scripts hard-code
the path `./configurations/<filename>` so the working directory must be the LEEana root.
To run a non-default analysis, copy or symlink the desired `configurations/<subdir>/`
files into `configurations/`.

---

## cv_input.txt

**Consumers:** `convert_histo.pl` → `bin/convert_checkout_hist[_xs]`.

**Schema** (whitespace-separated; columns positional):

| Col | Field | Notes |
|-----|-------|-------|
| 0 | `#file` | Integer filetype code: 1=intrinsic_nue, 2=nu_overlay, 3=EXTBNB, 4=DIRT, 5=BNB data. Code 15=scaled data variant. Readme: BNB data **must** be code 5. |
| 1 | `#type` | String label for logging (e.g. `intrinsic_nue`, `BNB`). |
| 2 | `#period` | Run period integer (1, 2, 3). |
| 3 | `#filename` | Path to the processed checkout ROOT file (input). |
| 4 | `#outfile_name` | Path to the output histogram ROOT file. |
| 5 | `#ext_pot` | External (beam-off) POT in units of protons on target. `0` for MC. For EXTBNB and BNB data, enter the measured POT from `summarize_pot.pl`. |
| 6 | `#file_no` | Unique integer ID for this entry; used to cross-reference `file_ch.txt` and `xf_input.txt`. Must be unique within one analysis. |
| 7 | `#norm` | Normalisation flag (almost always 0). |
| 8 | `#norm_period` | Sub-period normalisation for split-run analyses. |

**Sentinel:** A row whose first field is `-1` terminates parsing. `convert_histo.pl:13`
stops at the first `-1` line.

**Important:** The root `configurations/cv_input.txt` contains **multiple blocks**
separated by `-1 end ...` sentinel rows. Only the first block (lines 1–15) is ever read;
lines 17–52 are completely dead. Editing the bottom of the file has no effect.
See bug L-10.

**Column count:** 9 data columns (0–8). The header comment line at the bottom of each
block (starting with `#file`) has a column count of 9, consistent.

---

## cov_input.txt

**Consumers:** `bin/convert_checkout_hist[_xs]`, `bin/merge_hist[_xs]`, all
`*_cov_matrix` binaries.

**Schema:**

| Col | Field | Notes |
|-----|-------|-------|
| 0 | `Name` | Channel name (string, no spaces). Keywords in the name drive special-casing in `merge_hist.cxx`: substring `_ext_` → EXT channel, `_dirt_` → dirt channel, `_LEE_` → apply LEE weight. |
| 1 | `#Var` | ROOT expression to project (e.g. `kine_reco_Enu`). Must be a valid TTree branch expression accessible via `TTree::Project`. |
| 2 | `#bin` | Number of histogram bins (integer). |
| 3 | `#start` | Histogram lower edge (float, in the variable's unit). |
| 4 | `#end` | Histogram upper edge (float). |
| 5 | `#obs` | Observation category: 1 = FC (fully contained), 2 = PC (partially contained). Merged channels with the same `#obs` are summed into `hdata_obsch_N` / `hmc_obsch_N` in `merge_hist`. |
| 6 | `#xs_flux` | Whether this channel enters the flux/xs covariance (0/1). |
| 7 | `#det` | Whether this channel enters the detector covariance (0/1). |
| 8 | `#add_sys` | Additional fractional systematic applied to the diagonal (e.g. 0.5 for dirt = 50% normalisation uncertainty). |
| 9 | `#same_mc` | Channels with the same non-zero `#same_mc` value share MC statistical covariance (they come from the same MC sample). |
| 10 | `#cov_sec` | Index of this channel's position in the collapsed covariance matrix. Channels with the same index are collapsed together. |
| 11 | `#file` | Filetype code (matches col 0 of `cv_input.txt`). Links the histogram to the correct input file. |
| 12 | `#weight` | Weight recipe: `unity` (no event weight), `cv_spline` (apply GENIE tune weight `weight_cv`), `spline` (apply a polynomial spline weight). |
| 13 | `#lee_strength` | LEE signal multiplier applied at histogram-fill time. Normally 0; for LEE signal channels set to 1. |

**Sentinel:** A row whose `Name` field is `End` (capital E) terminates parsing.

**Example row:**
```
numuCC_Enu_mu_FC_bnb    kine_reco_Enu    25    0    2500    1    0    0    0    0    0    5    unity    0
```
→ channel `numuCC_Enu_mu_FC_bnb`, project `kine_reco_Enu` into 25 bins 0–2500 MeV,
FC category, no xs/det cov, no add_sys, no same_mc grouping, cov position 0, filetype 5
(BNB data), no weight, no LEE.

**Note:** The file must have a matching channel row for every (filetype × add_cut)
combination used in `file_ch.txt`. Unmatched combinations are silently ignored.

---

## file_ch.txt

**Consumers:** `bin/convert_checkout_hist[_xs]`.

**Schema** (2 columns):

| Col | Field | Notes |
|-----|-------|-------|
| 0 | `#file` | Full path to a processed checkout ROOT file; must match the `#filename` column in `cv_input.txt` exactly (string equality). |
| 1 | `#cut` | The `add_cut` name to apply (e.g. `all`, `numuCC`, `nueCC`, `LowEnu`). Multiple rows with the same file produce multiple cut variants. |

**Sentinel:** `end end` (all lowercase, two columns).

**Critical:** The root `configurations/file_ch.txt` also contains multiple blocks
separated by `end end` sentinels and header comments. Only the first block (lines 1–46)
is active; lines 48–87 are dead (second block, used for a different analysis run).

**Case mismatch with xf_file_ch.txt:** The corresponding `xf_file_ch.txt` uses
`End End` (capital E) as the sentinel, whereas `file_ch.txt` uses `end end`. Any parser
that uses the same case-sensitive comparison for both will misread one file.
See bug L-19.

---

## xf_input.txt / xf_file.txt

**Consumers:** `run_xf_sys.pl` → `bin/xf_cov_matrix`; `merge_weight.pl` → `bin/merge_xf`.
`check_xf.pl` reads `xf_file.txt` but uses the wrong column. See bug L-18.

The file is referred to as both `xf_input.txt` and `xf_file.txt` in different contexts.
The actual filename depends on which analysis subdir is staged.

**Schema** (whitespace-separated):

| Col | Field | Notes |
|-----|-------|-------|
| 0 | `#file` | Filetype code (matches `cv_input.txt` col 0). |
| 1 | `#type` | String label. |
| 2 | `#weight-index` | Systematic knob index 1–17 (selects which of the 17 `xf_cov_matrix` runs this row belongs to). |
| 3 | `#weight-rootfile` | Path to the merged reweight ROOT file (output of `bin/merge_xf`). |
| 4 | `#cov-output` | Output path for the covariance ROOT file. |
| 5 | `#ext_pot` | External POT (0 for MC). |
| 6 | `#file_no` | File number (matches `cv_input.txt` col 6). |
| 7 | `#norm-period` | Sub-period normalisation. |

**Sentinel:** A row whose first field is `-1` terminates parsing.

**The 17 systematic knobs** (index 1–17):
1. expskin_FluxUnisim
2. horncurrent_FluxUnisim
3. kminus_PrimaryHadronNormalization
4. kplus_PrimaryHadronFeynmanScaling
5. kzero_PrimaryHadronSanfordWang
6. nucleoninexsec_FluxUnisim
7. nucleonqexsec_FluxUnisim
8. nucleontotxsec_FluxUnisim
9. piminus_PrimaryHadronSWCentralSplineVariation
10. pioninexsec_FluxUnisim
11. pionqexsec_FluxUnisim
12. piontotxsec_FluxUnisim
13. piplus_PrimaryHadronSWCentralSplineVariation
14. reinteractions_piminus_Geant4
15. reinteractions_piplus_Geant4
16. reinteractions_proton_Geant4
17. UBGenieFluxSmallUni

---

## xf_file_ch.txt

**Consumers:** `bin/xf_cov_matrix` (analogous to `file_ch.txt` for the reweight path).

**Schema:** Identical 2-column format to `file_ch.txt`.
**Sentinel:** `End End` (capital E — differs from `file_ch.txt`'s `end end`). See bug L-19.

---

## gp_input.txt

**Consumers:** `bin/det_cov_matrix -g1` → `GPSmoothing.C` → `GPRegressor.cxx`.

Format: 7 rows followed by `#END` sentinel.

| Row | Content |
|-----|---------|
| 1 | 5 booleans (`true`/`false`) — whether to use log-scale for each of the 5 GP dimensions. Example: `false false true false false` means log-scale only for dimension 3 (reco-Enu). |
| 2 | 5 length-scales followed by 1 kernel scaling parameter. `0` for a dimension disables cross-bin correlation in that dimension. Example: `0 0 1.2 0 0 1` — only dim 3 (Enu) active with length-scale 1.2. On log scale, 1.2 means ~20% smoothing per unit of log(Enu). |
| 3 | Bin centres for dimension 1 (sample type). Example: `0 1 2` for sig/bkg/ext. |
| 4 | Bin centres for dimension 2 (FC/PC). Example: `0 1`. |
| 5 | Bin centres for dimension 3 (reco-Enu). Example: `50 150 250 … 2450 2550` (26 bins at 100 MeV centres). |
| 6 | Bin centres for dimension 4 (unused in this config → `0`). |
| 7 | Bin centres for dimension 5 (unused → `0`). |

**Constraints:**
- Row 5 must have exactly as many values as there are Enu bins in the analysis.
  If the `cov_input.txt` binning changes, `gp_input.txt` must be updated manually.
- `GPSmoothing.C` hardcodes dimension 5 as `GPPoint double x[5]`; requesting >5
  dimensions silently overwrites stack memory.

---

## det_file.txt / det_input.txt (not committed at root)

These files are referenced by `merge_det.pl` and `bin/det_cov_matrix` respectively.
They must be manually staged from `configurations/<analysis>/det_file.txt` and
`configurations/<analysis>/det_input.txt` before running detector systematics.

`det_file.txt` lists the (CV file, DetVar file) pairs for `bin/merge_det`:
```
<cv_file>  <detvar_file>  <output_file>  [<period>]  [<run_flag>]
```

`det_input.txt` lists the detector systematic source files with their covariance
output names (analagous schema to `xf_input.txt`).

---

## Summary: sentinel strings

| File | Sentinel | Notes |
|------|---------|-------|
| `cv_input.txt` | First column == `-1` (numeric) | Stops after first block; subsequent blocks are dead |
| `cov_input.txt` | `Name` == `End` (capital E) | Must match exactly |
| `file_ch.txt` | `end end` (all lowercase) | |
| `xf_file_ch.txt` | `End End` (capital E) | Differs from `file_ch.txt` — see bug L-19 |
| `xf_input.txt` | First column == `-1` (numeric) | |
| `gp_input.txt` | `#END` | First character `#` triggers stop |
| `filelist_bdt` | First column == `"end"` (string compare) | |
| `filelist_cv` | First column == `"end"` (string compare) | |
| `merge_det.pl` file | First column == `"end"` (string compare) | |
| `merge_weight.pl` file | First column == `'end'` (single-quoted) | |
