# LEEana Analysis Code Examination

**What this is:** A read-only static audit of the LEEana repository
(`/nfs/data/1/xqian/prototype-dev/LEEana`) and its companion C++ library
(`/nfs/data/1/xqian/prototype-dev/wcp-uboone-bdt`), which together implement the
MicroBooNE Wire-Cell Pandora (WCP) post-reconstruction analysis framework used for
the Low-Energy Excess (LEE) search and cross-section measurements.
No source files were modified. Completed 2026-04-19.

---

## Companion examination

A separate, detailed static audit of the **wcp-uboone-bdt** C++ source was completed
on 2026-04-14 and is located at:

```
/nfs/data/1/xqian/prototype-dev/wcp-uboone-bdt/docs/examinations/
```

That audit contains 8 documents and catalogues 26 bugs (B-01 through B-26). The present
examination focuses on the LEEana orchestration layer (Perl drivers, ROOT macros,
configuration files, `wiener_svd/`, `plot_script/`), adds **new** findings from deeper
reads of the C++ apps and library, and provides a unified algorithm narrative that spans
both repositories.

Bug IDs from the sibling audit are referenced as **B-xx**.
New findings in this audit are labelled **L-xx**.

---

## Reading order

| # | Document | Contents |
|---|----------|----------|
| 1 | [01_overview.md](01_overview.md) | Start here. End-to-end pipeline flow diagram, artifact map, driver-to-binary table. |
| 2 | [02_algorithms_general.md](02_algorithms_general.md) | Physics goals (LEE search, 3+1 oscillation, xs measurement, NuMI constraint) and conceptual overview of each major algorithm without heavy math. |
| 3 | [03_algorithms_details.md](03_algorithms_details.md) | Mathematical detail: CNP χ², Schur-complement constraint, Feldman-Cousins, covariance construction, Bayesian MC-stat, Gaussian-process smoothing, Wiener-SVD unfolding — all with file:line citations. |
| 4 | [04_pipeline_drivers.md](04_pipeline_drivers.md) | Per-driver reference for every Perl script and ROOT macro in LEEana, and the wiener_svd/ stage. |
| 5 | [05_configurations.md](05_configurations.md) | Schema reference for every configuration file parsed by the analysis: cv_input.txt, cov_input.txt, file_ch.txt, xf_input.txt, xf_file_ch.txt, gp_input.txt. |
| 6 | [06_bugs.md](06_bugs.md) | Prioritized bug catalogue (High / Medium / Low / Style). Read the High items first — they can produce silently wrong physics. |
| 7 | [07_efficiency.md](07_efficiency.md) | Performance and resource-use findings: fan-out races, redundant I/O, covariance rebuild cost, branch-address overhead, FFT sizing. |

---

## Bug counts (06_bugs.md)

| Severity | Count | Examples |
|----------|-------|---------|
| High     | 7  | L-01 fan-out race (no `wait`), L-02 `pot_counting.cxx` end-iterator deref, L-03 `plot_hist.cxx` TString truncation, L-04 `det_cov_matrix.cxx` bootstrapping typo, L-05 `prune_weightsep24_trees` unguarded `at()`, L-06 GP point in-place mutation [dup B-02], L-07 Util MatrixMatrix/MatrixMatirx mismatch [dup B-22] |
| Medium   | 8  | L-08 `convert_cv.cxx` operator-precedence, L-09 `convert_histo.pl` numeric `!=` on string, L-10 `cv_input.txt` dead-tail sentinel trap, L-11 `merge_hist.cxx` TFile FD leak in loop, L-12 `stat_cov_matrix.cxx` hard-coded output path, L-13 `det_cov_matrix.cxx` 25% hard-coded zero floor, L-14 `run_gof.pl` stat_pred race, L-15 `argv[2]` OOB in `pot_counting.cxx` |
| Low      | 7  | L-16 `merge_numi_flux_geom.pl` host-specific paths, L-17 `summarize_pot.pl` host-specific paths, L-18 `check_xf.pl` wrong column index for xf_input schema, L-19 `file_ch.txt` vs `xf_file_ch.txt` terminator mismatch, L-20 `run_det_sys.pl` oscillation mode runs all 10 sources, L-21 prune_weight copy-paste family divergence, L-22 `WienerSVD_3D.C` debug prints in production |
| Style    | 5  | L-23 `applyNuMIGeomtryWeights` typo in binary name, L-24 readme `pot_countng_mc` typo, L-25 readme `./configuration/` singular, L-26 `run_xs.pl` comment says 1→16 (should be 1→17), L-27 Feldman spelling variants in TLee API [dup B-23] |
| **Total** | **27** | |

---

## Severity rubric

- **High** — the defect produces silently wrong physics outputs, a crash on common inputs, or an
  undetected race condition that corrupts outputs. Fix before any result is used for publication
  or unblinding.
- **Medium** — latent or conditional defects that corrupt results in identifiable circumstances
  (specific input values, call order, file count). Fix before production runs on new datasets.
- **Low** — brittle assumptions or hard-coded constants that silently break on dataset changes
  (new runs, changed binning, different host). Fix as part of any analysis extension.
- **Style** — naming or dead-code issues with no runtime consequence. Fix opportunistically.

---

## Scope limitations

- `plot_script/*.C` — ~50 macros; deep-dive only for
  `plot_check_{det,xf,stat,gof,nueCCFC,numuCCFC,xs}.C`. The remainder are surveyed structurally.
- `inc/WCPLEEANA/tagger.h` — 2994-line BDT variable struct; structural survey only.
- `apps/old/` — excluded from the wcp-uboone-bdt build; not read.
- **No runtime execution** — all findings are static, based on source reading. I/O performance
  numbers are analytical estimates.
- **No git history** — no examination of commit authorship or prior file versions.
- **No external validation** — algorithm descriptions reflect what the code does, not what any
  physics paper says it should do. Discrepancies with a tech note are flagged as open questions.
