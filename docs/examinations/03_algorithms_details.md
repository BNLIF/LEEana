# 03 — Algorithms: Mathematical Detail

Cross-references to the companion examination:
`wcp-uboone-bdt/docs/examinations/07_algorithms.md` — contains extended derivations
for CNP, constraint, Feldman-Cousins, and Wiener-SVD. The sections below provide
enough detail to understand the implementation, with file:line citations.

---

## 1 — CNP chi-squared (TLee.cxx)

The analysis uses the **Combined Neyman-Pearson (CNP)** chi-squared statistic, which
avoids the divergence of the standard Neyman χ² at zero data counts.

The CNP effective variance for bin i is:

```
σ²_CNP(i) = 3·N_obs(i)·N_pred(i) / (2·N_obs(i) + N_pred(i))
```

Limiting cases:
- N_obs = 0 → σ²_CNP = N_pred / 2  (Poisson from prediction)
- N_obs ≫ N_pred → σ²_CNP → N_obs  (Neyman limit)
- N_obs ≈ N_pred → σ²_CNP → N/2 · (something between N_obs and N_pred)

When N_pred = 0 and N_obs = 0, a floor of 1e-6 is applied to avoid division by zero.

**Implementation:** `wcp-uboone-bdt/src/TLee.cxx`
- `Minimization_Lee_strength_FullCov`: lines 219–278 (FCN lambda building the CNP
  stat covariance matrix every Minuit evaluation).
- `GetChi2`: lines 537–570 (standalone χ² evaluation).
- `Plotting_singlecase`: lines 574–608 (duplicate construction — drift risk).

The CNP stat variance is added to the systematic covariance diagonal before inversion:

```
M_total = M_CNP_stat + M_sys_cov
χ² = (data - pred)^T · M_total⁻¹ · (data - pred)
```

Inversion is via `TMatrixD::Invert()` without rank or condition check
(`TLee.cxx:262–278` and `TLee.cxx:1580`). See bug L-XX in `06_bugs.md`.

---

## 2 — Sideband constraint (Schur-complement)

The fit is performed in two stages. The numuCC sideband channels (FC + PC) are used
to constrain the flux/cross-section prior before evaluating the LEE signal region.

Let the prediction vector µ and covariance matrix Σ be partitioned as:

```
µ = [µ_signal ; µ_sideband]
Σ = [Σ_SS  Σ_SB]
    [Σ_BS  Σ_BB]
```

Given observed sideband data d_B, the conditional signal prediction is:

```
µ_S|B = µ_S + Σ_SB · Σ_BB⁻¹ · (d_B - µ_B)
Σ_S|B = Σ_SS - Σ_SB · Σ_BB⁻¹ · Σ_BS
```

This is the Schur complement of Σ_BB in Σ.

**Implementation:** `wcp-uboone-bdt/src/TLee.cxx`
- The constraint flag is `flag_Lee_minimization_after_constraint`.
- Partitioning and Schur solve: lines 280–407.
- There is an `if(0){…}` dead block at line 280 that duplicates the live `if(1){…}`
  code with hardcoded channel indices 26+26 and 137. This dead code will silently
  diverge from the live path as channel configurations change.

---

## 3 — LEE signal-strength minimisation

The LEE signal strength α is fitted by Minuit2 (MIGRAD, tolerance 1e-6, precision 1e-18)
minimising the χ² as a function of α. The signal contribution is:

```
N_pred(i; α) = N_SM(i) + α · N_LEE(i)
```

where N_LEE(i) = leeweight(Enu) × N_nue_intrinsic(i).

`leeweight(Enu)` is a hardcoded piecewise lookup at `master_cov_matrix.cxx:27`.

The full covariance M is rebuilt from scratch at every Minuit function evaluation
because the CNP stat variance depends on N_pred(i; α). The systematic covariance
component (det + xf + xs + add_cov) is α-independent and could be cached, but is
currently not (efficiency finding E-01 in `07_efficiency.md`).

---

## 4 — Feldman-Cousins coverage

The Feldman-Cousins procedure constructs a 2D surface of (α_true, χ²_critical) such
that the coverage probability equals the nominal confidence level (90% or 68%).

For each value of α_true:
1. Generate N_toy pseudo-experiments by sampling from the predicted distribution
   (Gaussian smearing via `Set_toy_Variation`).
2. For each toy, run the minimisation to find the best-fit α and compute
   Δχ²(α_true, α_best_fit).
3. The χ²_critical at this α_true is the (1-CL) quantile of the Δχ² distribution.

**Implementation:** `wcp-uboone-bdt/src/TLee.cxx`
- `Exe_Feldman_Cousins`: line 151 (correct spelling).
- `Exe_Fledman_Cousins_Asimov`: line 98 (typo).
- `Exe_Fiedman_Cousins_Data`: line 44 (typo).
- Default N_toy = 500 (`read_TLee_v20.cxx:834`), run serially. No ROOT IMT parallelism.
- `Set_Collapse` is called inside the toy loop on each iteration (`TLee.cxx:848, 862`),
  re-collapsing the full covariance each toy. This could be cached once per α_true.

---

## 5 — Systematic covariance construction

### 5a — Detector systematics (mcm_1.h, det_cov_matrix.cxx)

For each of the 9 active detector variation sources (source index 1–10, skipping 5):

1. Load the CV and DetVar event sets from `merged_det_*.root` (produced by `merge_det`).
2. **Bootstrap phase** (1000 iterations, `mcm_1.h`):
   - Draw a bootstrap replica of the CV sample (Poisson resampling per event weight).
   - Use `TPrincipal` to find the principal components of the replica vs DetVar
     difference.
   - Accumulate the outer products into `cov_mat_bootstrapping`.
3. **Toy phase** (16000 draws):
   - Draw from the bootstrapped covariance (Cholesky decomposition) to produce toys.
   - Accumulate into `cov_det_mat`.
4. Fractional covariance: divide by the outer product of predicted means.
   For zero-prediction bins: if both means are zero → frac_cov = 0.
   If one mean is zero and covariance is nonzero and diagonal → frac_cov = 1/16 = 0.0625
   (25% fractional uncertainty, hardcoded at `det_cov_matrix.cxx:133`).

Optional GP smoothing via `GPSmoothing.C` (flag `-g1` or `-g3`) is applied to the
fractional covariance before the toy phase.

### 5b — Flux/XS/GENIE systematics (mcm_2.h, xf_cov_matrix.cxx)

For each of the 17 systematic knobs (expskin, horncurrent, kminus, kplus, kzero,
nucleoninexsec, nucleonqexsec, nucleontotxsec, piminus, pioninexsec, pionqexsec,
piontotxsec, piplus, reinteractions_piminus, reinteractions_piplus,
reinteractions_proton, UBGenieFluxSmallUni):

Multi-universe method: each MC event carries a vector of universe weights (typically
N_univ ~ 100). The covariance is computed as:

```
Σ_ij = (1/N_univ) · Σ_k [ (h_k(i) - h_nom(i)) · (h_k(j) - h_nom(j)) ]
```

where h_k(i) is the event count in bin i under weight universe k.

For the special case of 2-universe knobs (e.g. ±1σ), the code uses a different
normalisation (`mcm_2.h`: `if(nsize==2) temp_mat *= 1./2` vs standard `1./nsize`).

### 5c — Bayesian MC-stat covariance (bayes.cxx, merge_hist.cxx)

For each prediction bin i, let w_j be the per-event weight and n the number of events:

- Zero-measurement events (w_j but no data): contribute `weight² / count` to an
  effective "zero-measurement" variance.
- Nonzero data events: contribute via `add_meas_component(weight_val, weight_sq_val)`.

The posterior mean/variance of the Poisson rate is found by convolving the per-event
Poisson distributions using `TF1Convolution` (10,000-point FFT).

Credibility intervals are found by 30-iteration bisection (`bayes.cxx:229, 251`).

Key formula (`bayes.cxx:377`):
```
eff_weight = σ²/µ = weight_sq / weight_sum
eff_meas = µ/eff_weight = weight_sum² / weight_sq
f(x) = Poisson(eff_meas; x / eff_weight)  -- rescaled Poisson
```

The result is the MC-stat variance used as a diagonal entry in the collapsed covariance
(`merge_hist.cxx:237`). If `isnan(cov)` (FFT failure), falls back to `get_covariance_mc()`
(Gaussian approximation). If that is also zero, the bin gets stat error 0 — see bug L-XX.

### 5d — Covariance merging (merge_hist.cxx)

`bin/merge_hist` loads all per-file histograms, reads the MC-stat log files, reads the
det and xf covariance ROOT files, and writes:
- `mat_collapse` — the collapse matrix mapping from full to observation-channel space
  (used in TLee to compute the effective covariance in obs-ch space).
- `cov_mat_add` — the total systematic + stat covariance in full-channel space.
- `histo_N`, `hdata_obsch_N`, `hmc_obsch_N` — per-obs-channel histograms.

Channels tagged `_ext_` in their name are treated as EXT (beam-off) with Poisson stat.
Channels tagged `_dirt_` have an additional 50% normalisation uncertainty.
Channels tagged `_LEE_` have the eLEE weight applied.

---

## 6 — Wiener-SVD unfolding (WienerSVD.cxx, convert_wiener_simple.C)

Given:
- Measured background-subtracted data vector M (dimension N_meas)
- Response matrix A (N_meas × N_true)
- Covariance matrix Σ_M (N_meas × N_meas)
- Signal prior S (dimension N_true, e.g. from SM MC)

### Step 1: Scale normalisation

```
Q = diag(1/√Σ_M_ii)       -- rescale by measurement uncertainty
```

### Step 2: SVD decomposition

```
R = Q · A · C             -- C is the regularisation matrix (identity, diagonal, or
                              finite-difference for derivative-type regularisation)
R = U · D · V^T           -- economy SVD
```

Regularisation types in `WienerSVD.cxx`:
- Type 1: C = identity
- Type 2: C = diag(√signal) (signal-normalised)
- Type 22/32: C = 1st/2nd derivative matrix (smoothness regularisation)

The derivative matrices use hardcoded `dim_edges[]` at lines 40 and 47.

### Step 3: Wiener filter

```
S_rot = V^T · C · S_prior      -- rotate signal prior into SVD space
W_ii = S_rot(i)² / (D_ii² · S_rot(i)² + 1)   -- Wiener weights
```

When `flag_WienerFilter == 0` (no filter): W_ii = 1/D_ii² — numerically unstable for
small singular values.

### Step 4: Unfolding

```
x_unfold = C⁻¹ · V · W · D^T · U^T · Q · M
```

### Step 5: Smearing matrix and covariance transport

```
A_C = C⁻¹ · V · W₀ · V^T · C       -- W₀ uses prior-normalised Wiener weights
cov_unfold = A_C · C⁻¹ · cov_rot · (A_C · C⁻¹)^T
```

where `cov_rot = V^T · C · (Q · Σ_M · Q^T) · C^T · V`.

**Implementation:**
- Core SVD + filter: `wcp-uboone-bdt/src/WienerSVD.cxx` (236 lines).
- Macro that assembles inputs, calls `WienerSVD()`, writes `wiener.root`:
  `wiener_svd/convert_wiener_simple.C` (uses `merge_xs.root`).
- 3D variant: `wcp-uboone-bdt/src/WienerSVD_3D.C`.

**Key implementation concern:** `C0.Invert()` at `WienerSVD.cxx:163` is unchecked.
`normsig = 1/Signal(i)^Norm_type` at `WienerSVD.cxx:~95` has no guard for
Signal(i) == 0 (divide-by-zero for empty true bins).

---

## 7 — Gaussian-process smoothing (GPKernel.cxx, GPRegressor.cxx, GPSmoothing.C)

### Kernel

RBF (squared-exponential) kernel in up to 5 dimensions:

```
k(x, x') = σ² · exp( -Σ_d (x_d - x'_d)² / θ_d² )
```

where σ² is the overall scaling (`fPars[last]`) and θ_d is the length-scale in
dimension d. Log-scale option: if `log_scale[d]` is true, use `log(x_d)` instead of
x_d before computing the distance (configured in `gp_input.txt` row 1).

Zero-length-scale dimensions (`θ_d = 0`) are excluded from the sum — bins with
different values in that dimension are treated as uncorrelated (`k → 1e6` or → 0
depending on branch at `GPKernel.cxx:51`).

**Implementation:** `wcp-uboone-bdt/src/GPKernel.cxx` + `inc/WCPLEEANA/GPKernel.h`.

**Critical bug:** `GPKernel::RBFKernel::Mag()` mutates its GPPoint arguments in-place
at `GPKernel.cxx:51-52` (`p1x[i] = log(p1x[i])`). If GPPoints are shared across
calls, this logs the same coordinates multiple times (log-of-log), silently corrupting
distances. See bug L-06 / B-02.

### Hyperparameter optimisation

Hyperparameters (log length-scales and log σ²) are fitted by maximising the log
marginal likelihood:

```
log p(y | X, θ) = -½ y^T K⁻¹ y - ½ log|K| - (n/2) log(2π)
```

Gradient computed analytically:

```
∂ log p / ∂ θ_j = ½ tr[(α α^T - K⁻¹) ∂K/∂θ_j]
```

where α = K⁻¹ y. Optimised via GSL BFGS2, 50 iterations, log-scale parameter bounds
(−5, 5) (`GPRegressor.cxx:71–120`).

### Smoothing application

`GPSmoothing.C` calls the GP to predict smoothed fractional covariance values at each
(sample_type, FC/PC, Enu, ...) bin centre. The smoothed values replace the raw
bootstrap covariance entries before the toy phase in the detector covariance calculation.

---

## 8 — Oscillation weight (master_cov_matrix.cxx)

For 3+1 sterile fits, the survival probability P(νe → νe) is applied to intrinsic-nue
events as a per-event weight:

```
P(νe → νe; Δm², sin²2θ, L, Enu) = 1 - sin²2θ · sin²(1.267 · Δm² · L / Enu)
```

This is computed by `get_osc_weight(Enu, L)` in `master_cov_matrix.cxx:562` and
multiplied onto the event weight during histogram filling. The oscillation parameters
(Δm², sin²2θ) are read from `configurations/<analysis>/osc_parameter.txt`.
