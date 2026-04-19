# 01 — Pipeline Overview

## What LEEana is

LEEana is the **orchestration layer** for the MicroBooNE WCP analysis.
It does not implement the analysis algorithms — those live in the companion package
`wcp-uboone-bdt` which is compiled into binaries placed in `./bin/`.
LEEana supplies:

- Perl drivers that invoke those binaries in the right order.
- Configuration text files that parameterise the sample list, channel binning,
  and systematic-source assignments.
- ROOT macros for validation plots, Wiener-SVD unfolding, and comparison studies.

The two repositories together constitute one analysis pipeline.

---

## Source tree layout

```
LEEana/                          ← orchestration layer (this repo)
│
├── *.pl                         ← Perl driver scripts
├── *.C, DrawOption.cc           ← standalone ROOT macros
├── configurations/              ← config text files (one subdir per analysis)
│   ├── cv_input.txt             ← sample list  (schema: §05)
│   ├── cov_input.txt            ← channel / binning definitions
│   ├── file_ch.txt              ← per-file add_cut list
│   ├── xf_input.txt             ← xf/det weight file list
│   ├── xf_file_ch.txt           ← per-weight-file add_cut list
│   ├── gp_input.txt             ← GP smoothing parameters
│   └── <analysis>/              ← per-analysis variant overrides
├── plot_script/                 ← ROOT validation plot macros
├── wiener_svd/                  ← Wiener-SVD unfolding macros
├── genie/                       ← GENIE comparison macros
├── flux_info/numi/              ← NuMI flux-weight text files
├── training_list/               ← BDT training/test event lists
└── docs/examinations/           ← ← ← this directory

wcp-uboone-bdt/                  ← C++ library + compiled binaries
├── src/                         ← algorithm implementations
├── inc/WCPLEEANA/               ← headers (schema structs, algorithm APIs)
├── apps/                        ← one .cxx = one binary under ./bin/
└── docs/examinations/           ← companion examination (completed 2026-04-14)
```

---

## End-to-end pipeline

```
 Raw WCP "checkout" ROOT files
 (TTree wcpselection/{T_eval,T_BDTvars,T_KINEvars,T_PFeval,T_weight,T_pot})
          │
          │  OPTIONAL: update BDT scores
          │  convert_bdt.pl  ──►  bin/bdt_convert  ──►  checkout_rootfiles_correct_bdt/
          │
          ▼
 Step 1 — CV deduplication
          convert_cv.pl  ──►  bin/convert_cv_spec  (per file from filelist_cv)
          Remove duplicate events (run,subrun,event), flag failed subruns (>20%),
          rescale POT by pass_ratio
          Output: processed_checkout_rootfiles/checkout_*.root
          │
          │  POT accounting (standalone, run once)
          │  summarize_pot.pl  ──►  shell: cat|grep|awk  ──►  pot_bnb.txt, pot_extbnb.txt
          │  bin/pot_counting  #bnb_file #extbnb_file -m2  ──►  stdout (BNB + EXTBNB POT)
          │  bin/pot_counting_mc  #mc_file  ──►  stdout (MC POT)
          │  Result hand-copied into configurations/cv_input.txt (#ext_pot column)
          │
          ▼
 Step 2 — Histogram production (CV central value)
          convert_histo.pl [0|1|2]  reads configurations/cv_input.txt
          Mode 0 (default):   bin/convert_checkout_hist   ──►  hist_rootfiles/*.root
          Mode 1 (xs):        bin/convert_checkout_hist_xs ──►  hist_rootfiles/*.root
          Mode 2 (osc):       bin/convert_checkout_hist -o1 ──►  hist_rootfiles/*.root
          Consults: cov_input.txt (channels), file_ch.txt (per-file add_cuts)
          Output: one ROOT file per (MC/data file), holding per-channel TH1F + POT TTree
          │
          ├─── Step 3a — MC-prediction stat covariance (run once)
          │    bin/stat_pred_cov_matrix -r0  ──►  hist_rootfiles/run_pred_stat.root
          │
          ├─── Step 3b — Detector systematics (10 sources)
          │    merge_det.pl  ──►  bin/merge_det  (merges CV + DetVar checkouts)
          │          ──►  hist_rootfiles/DetVar/merged_det_*.root
          │    run_det_sys.pl  ──►  bin/det_cov_matrix -r1..10 (skips r=5 by default)
          │          ──►  hist_rootfiles/DetVar/cov_det_mat_*.root
          │
          ├─── Step 3c — Flux/XS/GENIE systematics (17 knobs)
          │    merge_weight.pl  ──►  bin/merge_xf  (merges CV + reweight checkouts)
          │          ──►  processed_checkout_rootfiles/<dir>/<weight>.root
          │    NuMI geometry special:
          │    merge_numi_flux_geom.pl  ──►  bin/applyNuMIGeomtryWeights [typo!]
          │    run_xf_sys.pl  ──►  bin/xf_cov_matrix -r1..17
          │          ──►  hist_rootfiles/XsFlux/cov_*.root
          │
          └─── Step 3d — MC-stat scan (for Bayesian MC-stat covariance)
               run_mc_stat.pl  ──►  100× bin/merge_hist -r0 -e2 -l$lee_strength
                     ──►  mc_stat/0.log .. mc_stat/99.log  (LEE strengths 0..2.97)
          │
          ▼
 Step 4 — Merge + collapsed covariance matrix
          bin/merge_hist -r0 -l$lee -e1   ──►  merge.root
          bin/merge_hist_xs -r0 -l0       ──►  merge_xs.root  (xs mode)
          Reads: all hist_rootfiles/, mc_stat/*.log, cov_*.root
          Output: per-observation-channel histograms, mat_collapse, cov_mat_add
          │
          ├─── TLee fit path (LEE search / GoF / Feldman-Cousins)
          │    bin/read_TLee_v20  ──►  reads merge.root + cov files
          │          ──►  LEE signal strength fit, χ² GoF, FC coverage
          │    bin/plot_hist -r0 -l0 -c1 -e3 -s<collapsed_cov_file>
          │          ──►  plots (PDF/ROOT)
          │    Validation: plot_script/plot_check_{det,xf,stat,gof,nueCCFC,...}.C
          │
          └─── Wiener-SVD unfolding path (cross-section measurements)
               wiener_svd/copy.pl  ──►  stages mc_stat, DetVar, XsFlux, merge_xs.root
               wiener_svd/convert_wiener_simple.C  ──►  wiener.root
               bin/wiener_example wiener.root output.root 2 0.5
               wiener_svd/plot*.C  ──►  cross-section plots
```

---

## Driver → binary → config table

| Driver | Config read | Binary invoked | Output artifact |
|--------|-------------|----------------|-----------------|
| `convert_bdt.pl` | `filelist_bdt` (cols 0+1=path, 2+1=out, 4=type, 5=run_period) | `bin/bdt_convert` | `checkout_rootfiles_correct_bdt/` |
| `convert_cv.pl` | `filelist_cv` (cols 2+1=in, 3+1=out) | `bin/convert_cv_spec` | `processed_checkout_rootfiles/` |
| `convert_histo.pl` | `configurations/cv_input.txt` (cols 3=in, 4=out); `cov_input.txt`; `file_ch.txt` | `bin/convert_checkout_hist[_xs]` | `hist_rootfiles/*.root` |
| `merge_det.pl` | `configurations/det_file.txt` (**not committed** — must stage manually) | `bin/merge_det` | `hist_rootfiles/DetVar/merged_*.root` |
| `merge_weight.pl` | `configurations/xf_file.txt` (**not committed**) | `bin/merge_xf` | `processed_checkout_rootfiles/<dir>/` |
| `merge_numi_flux_geom.pl` | hard-coded paths | `bin/applyNuMIGeomtryWeights` | NuMI FluxUnisim ROOT files |
| `run_det_sys.pl` | none | `bin/det_cov_matrix -r1..10` | `hist_rootfiles/DetVar/cov_det_mat_*.root` |
| `run_xf_sys.pl` | none | `bin/xf_cov_matrix -r1..17` | `hist_rootfiles/XsFlux/cov_*.root` |
| `run_mc_stat.pl` | none | `bin/merge_hist -r0 -e2 -l$i` ×100 | `mc_stat/0.log`..`mc_stat/99.log` |
| `run_xs.pl` | (calls sub-drivers) | sub-drivers + `bin/xs_cov_matrix -r17` + `bin/merge_hist_xs` | `merge_xs.root` |
| `run_numi.pl` | (calls sub-drivers) | sub-drivers + `bin/merge_hist -r0 -l1` | `merge.root` (NuMI mode) |
| `run_gof.pl` | (calls sub-drivers) | `bin/stat_pred_cov_matrix -r0` + sub-drivers + `bin/merge_hist -r0 -l0` | `merge.root` (GoF mode) |
| `summarize_pot.pl` | `pot_counting/data_*.txt` + `/data0/xqian/…` | `cat\|grep\|awk` shell pipeline | `pot_bnb.txt`, `pot_extbnb.txt` |
| `gen_training_list.pl` | hard-coded `./old_files/*.root` | `bin/gen_training_list` | `training_list/` |
| `check_failure.pl` | `filelist` (col 1=path) | `bin/check_failures` | stdout |
| `check_xf.pl` | `configurations/xf_file.txt` (col 2=path) | `bin/check_xf_weight_xs` | stdout |

---

## Data artifacts

| Artifact | Producer | Consumer |
|----------|----------|---------|
| `processed_checkout_rootfiles/checkout_*.root` | `convert_cv.pl` | `convert_histo.pl`, `merge_det.pl`, `merge_weight.pl` |
| `hist_rootfiles/*.root` | `convert_histo.pl` | `merge_det.pl`, `run_mc_stat.pl`, `merge_hist` |
| `hist_rootfiles/DetVar/merged_*.root` | `merge_det.pl` | `run_det_sys.pl` |
| `hist_rootfiles/DetVar/cov_det_mat_*.root` | `run_det_sys.pl` | `bin/read_TLee_v20`, `bin/merge_hist` |
| `hist_rootfiles/XsFlux/cov_*.root` | `run_xf_sys.pl` | `bin/read_TLee_v20`, `bin/merge_hist` |
| `mc_stat/*.log` | `run_mc_stat.pl` | `bin/merge_hist` (Bayesian MC-stat path), `wiener_svd/convert_wiener_simple.C` |
| `merge.root` | `bin/merge_hist` | `bin/read_TLee_v20`, `bin/plot_hist`, `plot_script/*.C` |
| `merge_xs.root` | `bin/merge_hist_xs` | `wiener_svd/copy.pl` → `convert_wiener_simple.C` |
| `wiener.root` | `wiener_svd/convert_wiener_simple.C` | `bin/wiener_example` |
| `output.root` | `bin/wiener_example` | `wiener_svd/plot*.C` |
| `pot_bnb.txt`, `pot_extbnb.txt` | `summarize_pot.pl` | `bin/pot_counting` |

---

## Configuration analysis sub-directories

Each `configurations/<subdir>/` holds analysis-specific overrides of `cv_input.txt`,
`cov_input.txt`, `file_ch.txt`, etc. The driver scripts hard-code
`./configurations/cv_input.txt` (no subdir); users must manually copy/symlink the
desired subdir's files to `configurations/` before running.

| Subdir | Analysis |
|--------|---------|
| `0pNp` | 0-proton vs N-proton proton-multiplicity split for LEE |
| `3d_xs` | 3D differential cross-section (Enu × cos θ × …) |
| `bnb_constrain_numi` | Joint BNB+NuMI fit; BNB sideband constrains NuMI prediction |
| `bnb_numi_3plus1` | 3+1 sterile-ν oscillation using BNB+NuMI combined |
| `ehadron_xs` / `new_ehadron_xs` | Hadronic-energy differential xs |
| `emuon_xs` / `emuon_xs_2D` | Muon-energy 1D / 2D (Eµ × cos θµ) xs |
| `far_sideband` | Far-sideband blinded region channels |
| `generic_nu` | Generic inclusive CC+NC neutrino channels |
| `gof_test` | GoF validation configuration |
| `lee_fit` | Default LEE signal-strength fit (eLEE) |
| `model_validation_dl` | Deep-learning model validation |
| `nc_ch_example` | NC channel example |
| `numi_7ch` | NuMI 7-channel analysis |
| `numi_kdar` | NuMI KDAR-neutrino channels |
| `numi_nue_tot_xs` | NuMI nue total cross-section |
| `numi_tot_xs` | NuMI total cross-section |
| `numu_vtx` | numuCC vertex-dependent study |
| `old_weights`/`new_weights`/`weights` | Historical GENIE weight recipe comparisons |
| `reweighting` | GENIE tune reweighting study |
| `slope_com` | Shape/slope comparison |
| `technote_variables` | Tech-note kinematic distributions |
| `tot_xs` / variants | Inclusive total xs (several date-stamped variants) |
| `xs_1D_enu_crosscheck` | 1D Enu xs cross-check |
| `xs_3D` | 3D differential xs |

---

## Compiled binaries (wcp-uboone-bdt/apps/*.cxx → ./bin/)

The `wscript` (waf build) auto-discovers every `apps/*.cxx` and names the binary after
the stem. Key binaries and their role:

| Binary | App source | Role |
|--------|-----------|------|
| `convert_checkout_hist[_xs]` | apps/convert_checkout_hist[_xs].cxx | Build per-file histograms |
| `convert_cv_spec` | apps/convert_cv_spec.cxx | Dedup + fail-filter CV files |
| `merge_hist[_xs]` | apps/merge_hist[_xs].cxx | Merge + propagate uncertainties |
| `merge_det` | apps/merge_det.cxx | Merge CV + DetVar checkouts |
| `merge_xf` | apps/merge_xf.cxx | Merge CV + reweight checkouts |
| `det_cov_matrix` | apps/det_cov_matrix.cxx | Bootstrap detector cov |
| `xf_cov_matrix` | apps/xf_cov_matrix.cxx | Multi-universe flux/xs cov |
| `xs_cov_matrix` | apps/xs_cov_matrix.cxx | POT+target xs cov |
| `stat_cov_matrix` | apps/stat_cov_matrix.cxx | Data stat bootstrap cov |
| `stat_pred_cov_matrix` | apps/stat_pred_cov_matrix.cxx | MC stat bootstrap cov |
| `read_TLee_v20` | apps/read_TLee_v20.cxx | TLee fit driver |
| `plot_hist[2]` | apps/plot_hist[2].cxx | Diagnostic plots |
| `pot_counting[_mc]` | apps/pot_counting[_mc].cxx | POT accounting |
| `bdt_convert` | apps/bdt_convert.cxx | Re-apply BDT scores |
| `applyNuMIGeomtryWeights` | apps/applyNuMIGeomtryWeights.cxx | NuMI geometry weights [typo] |
| `wiener_example` | (in wcp-uboone-bdt/src/) | Run Wiener-SVD unfolding |
