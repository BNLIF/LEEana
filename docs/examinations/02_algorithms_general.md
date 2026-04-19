# 02 — Algorithms: Physics Goals and Conceptual Overview

## Physics context

MicroBooNE is a liquid-argon time projection chamber (LArTPC) at Fermilab that recorded
neutrino interactions from both the Booster Neutrino Beam (BNB) and the NuMI beam.
This analysis code implements several overlapping physics analyses:

1. **Low-Energy Excess (LEE) signal-strength fit** — test whether the data show an
   excess of νe-like low-energy events above the Standard-Model prediction, parameterised
   by a multiplicative "LEE strength" α. The nominal MiniBooNE-motivated signal hypothesis
   is α = 1 (eLEE model). The fit measures α and places confidence intervals.

2. **Goodness-of-fit (GoF)** — independent of the LEE signal, test whether the SM
   prediction is consistent with data across all analysis channels simultaneously, using
   a χ² statistic that properly accounts for the full correlated covariance matrix.

3. **Feldman-Cousins coverage** — construct frequentist confidence intervals on α by
   building a FC surface: for each (α_true, χ²_critical) pair the coverage probability is
   verified with toy MC experiments.

4. **Inclusive and differential neutrino cross-section measurements** — measure the
   total νµ CC cross section (and differential distributions in Eµ, Ehadron, Enu) using
   a Wiener-SVD unfolding procedure. Cross sections are extracted in bins of a
   reconstructed kinematic variable and corrected for detector smearing and efficiency.

5. **3+1 sterile oscillation fit** — a joint BNB+NuMI fit in which the NuMI channels
   constrain the near-detector fluxes, while the BNB channels provide the far-detector
   sensitivity. The oscillation probability P(νe→νe; Δm², sin²2θ) is applied event-by-event
   as a weight from `get_osc_weight()` in `master_cov_matrix.cxx`.

6. **NuMI constraint fit** — constrain the NuMI flux prediction using the BNB-observed
   numuCC rate before using NuMI nue events for LEE tests or cross-section measurements.

---

## Sample categories

The analysis uses five filetype codes (defined in `convert_checkout_hist.cxx` and
`cv_input.txt`):

| Code | Description |
|------|-------------|
| 1 | Intrinsic nue signal MC overlay |
| 2 | BNB nu overlay (the main background MC) |
| 3 | EXT-BNB (beam-off / cosmic data) |
| 4 | Dirt (neutrinos from outside the cryostat) |
| 5 | BNB data |

Code 15 is a scaled/fake-data variant. The analysis also has codes 6–10 for NuMI
variants in some configuration subdirs.

---

## Channel concept

A **channel** is one (histogram × add_cut × weight) triple:
- A histogram is a 1D distribution of one kinematic variable (e.g. `kine_reco_Enu`)
  in one observation category (FC = fully contained, PC = partially contained).
- The **add_cut** selects a signal or background sub-region (e.g. `numuCC_signal`,
  `nueCC_FC`, `LEE`).
- The **weight** specifies how event weights are applied (`unity`, `cv_spline`, `spline`).

The full channel list is in `configurations/cov_input.txt`. Channels are indexed by
`#cov_sec` (covariance matrix position) and `#obs` (observation category).
Special name substrings trigger hard-coded behaviour in `merge_hist.cxx`:
`_ext_` → EXT sample, `_dirt_` → dirt sample, `_LEE_` → apply LEE signal weight.

---

## How uncertainties are handled

Uncertainties are propagated through a **full covariance matrix** that combines:

1. **Data statistical uncertainty** — bootstrapped from the data histogram in
   `stat_cov_matrix.cxx` → `mcm_data_stat.h`.
2. **MC prediction statistical uncertainty** — bootstrapped from the MC histogram
   weights in `stat_pred_cov_matrix.cxx` → `mcm_pred_stat.h`.
   For the LEE fit, this uses the Bayesian convolution in `bayes.cxx` (iterated over
   100 LEE strengths 0..2.97 in `run_mc_stat.pl`).
3. **Detector systematics** — bootstrap procedure comparing CV vs 9 detector variation
   samples (SpaceCharge, LYDown, LYRayleigh, WireModX, WireModYZ, WireModThetaXZ,
   WireModThetaYZ, Recombination, LYAttenuation; source i=5 WireModdEdx skipped by
   default). Implemented in `det_cov_matrix.cxx` calling `mcm_1.h`.
4. **Flux, cross-section, and GENIE systematics** — multi-universe reweighting with 17
   systematic knobs covering pion/kaon production, nuclear reinteractions, horn current,
   and UBGenie GENIE tune. Implemented in `xf_cov_matrix.cxx` calling `mcm_2.h`.
5. **Additional fractional systematics** — a per-channel `#add_sys` constant applied
   to diagonal covariance entries (e.g. 0.5 for the dirt sample, reflecting 50%
   normalisation uncertainty).

All contributions are summed into one collapsed covariance matrix by `merge_hist.cxx`
before passing to `TLee`.

---

## LEE signal model

The eLEE (electron-neutrino LEE) signal is parameterised through an energy-dependent
scaling function `leeweight(Enu)` defined in `master_cov_matrix.cxx` (line 27).
This function returns a multiplicative factor applied to intrinsic-nue overlay events
as a function of neutrino energy, encoding the spectral shape predicted by the MiniBooNE
excess extrapolation. The overall strength is controlled by a global `lee_strength`
parameter α, which scales the eLEE weight: events are reweighted by 1 + α × leeweight(Enu).

---

## Sideband constraint

Before fitting the LEE signal region, the numuCC sideband is used to constrain the
flux/cross-section prediction (the Schur-complement / Schwartz-decomposition method).
The predicted mean and covariance are partitioned into signal+sideband blocks. The
conditional distribution of the signal prediction given the observed sideband data is
computed analytically: µ_signal|sideband = µ_signal + Σ_SB · Σ_BB⁻¹ · (data - µ_B),
tightening the prediction before the LEE search. This is implemented in `TLee.cxx`.

---

## Wiener-SVD unfolding

The cross-section measurement starts from the background-subtracted data histogram M
(data minus EXT minus dirt minus non-signal MC) and the response matrix R (smearing ×
efficiency × acceptance × flux × target nucleons). The Wiener-SVD algorithm decomposes R
via SVD, applies a Wiener filter W that regularises the inversion according to an assumed
signal prior, and produces an unfolded cross-section vector. The output includes a
smearing matrix A_C that characterises residual regularisation bias, and a propagated
covariance. Both the unfolded result and A_C are used in plotting and comparison with
theory predictions. Implemented in `WienerSVD.cxx`; called from `wiener_svd/convert_wiener_simple.C`
and `bin/wiener_example`.

---

## Gaussian-process smoothing

Detector covariance matrices are computed from 1000 bootstrap replicas comparing the
CV prediction to one detector-variation sample. With limited MC statistics, individual
bootstrap covariance entries are noisy. A 5-dimensional Gaussian Process regression
(dimensions: sample type, FC/PC, reco-Enu, plus up to two more) is optionally applied to
smooth the fractional covariance across kinematic bins before use. The GP uses an RBF
(squared-exponential) kernel with per-dimension length-scales and a log-scale option in
dimension 3. Hyperparameters are fitted by maximising the log marginal likelihood using
GSL BFGS2. Configured via `gp_input.txt`; implemented in `GPKernel.cxx`, `GPRegressor.cxx`,
`GPSmoothing.C`.

---

## Bayesian MC-stat convolution

To model MC statistical uncertainty analytically (rather than by Gaussian approximation),
`bayes.cxx` constructs the posterior distribution of the predicted event count per bin
as the convolution of multiple Poisson-with-weight components using `TF1Convolution`
with 10,000-point FFT evaluation. For bins with zero observed data, the effective
variance is Npred/2 (CNP). For bins with both data and prediction, the effective variance
is σ² = 3·Nobserved·Npredicted / (2·Nobserved + Npredicted). Credibility intervals
are found by bisection. The resulting per-bin variances are used as diagonal entries in
the Bayesian MC-stat covariance matrix.
