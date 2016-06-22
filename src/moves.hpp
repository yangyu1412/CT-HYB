#pragma once

#include <cmath>
#include <limits.h>
#include <math.h>

#include <boost/assert.hpp>
#include <boost/optional.hpp>
#include <boost/math/special_functions/factorials.hpp>
#include <boost/math/special_functions/binomial.hpp>
#include <boost/range/algorithm.hpp>

#include "operator.hpp"
#include "operator_util.hpp"
#include "wide_scalar.hpp"
#include "update_histogram.hpp"
#include "accumulator.hpp"
#include "sliding_window.hpp"

/**
 * @brief Change flavors of operators
 */
struct ExchangeFlavor {
  ExchangeFlavor(int *first) : first_(first) { }
  psi operator()(const psi &op) const {
    psi op_new = op;
    op_new.set_flavor(
        first_[op.flavor()]
    );
    return op_new;
  }
 private:
  int *first_;
};

/**
 * @brief Try to shift the positions of all operators (in imaginary time) by random step size.
 *
 * This update is always accepted if the impurity model is translationally invariant in imaginary time.
 * If you introduce a cutoff in outer states of the trace, it may not be always the case.
 * This update will prevent Monte Carlo dynamics from getting stuck in a local minimum in such cases.
 */
struct OperatorShift {
  OperatorShift(double beta, double shift) : beta_(beta), shift_(shift) { }
  psi operator()(const psi &op) const {
    assert(shift_ >= 0.0);
    psi op_new = op;

    double new_t = op.time().time() + shift_;
    if (new_t > beta_) {
      new_t -= beta_;
    }
    assert(new_t >= 0 && new_t <= beta_);

    OperatorTime new_time(op.time());
    new_time.set_time(new_t);
    op_new.set_time(new_time);
    return op_new;
  }
 private:
  double beta_, shift_;
};

/**
 * @brief pick one of elements randombly
 */
template<typename T>
inline
const T &pick(const std::vector<T> &array, alps::random01 &rng) {
  return array[static_cast<int>(rng() * array.size())];
}

/**
 * @brief pick a n elements randombly from 0, 1, ..., N-1
 */
template<class R>
std::vector<int> pickup_a_few_numbers(int N, int n, R &random01) {
  std::vector<int> flag(N, 0), list(n);

  for (int i = 0; i < n; ++i) {
    int itmp = 0;
    while (true) {
      itmp = static_cast<int>(random01() * N);
      if (flag[itmp] == 0) {
        break;
      }
    }
    list[i] = itmp;
    flag[itmp] = 1;
  }
  return list;
}

/*
template<unsigned int k>
class Combination {
  Combination(unsigned int N) : N_(N), smallest_elem_(0), comb_Nm1_(N - 1), done_(false) { }

  void reset(unsigned N) {
    N_ = N;
    smallest_elem_ = 0;
    comb_Nm1_ = N - 1;
    done_ = false;
  }

  unsigned int get(unsigned int idx) const {
    return idx == 0 ? smallest_elem_ : comb_Nm1_.get(idx - 1);
  }

  bool next_combination() {
    if (comb_Nm1_.done()) {
      ++smallest_elem_;
      if (smallest_elem_ > N_ - k) {
        done_ = true;
        return false;
      } else {
        comb_Nm1_.reset(N_ - smallest_elem_ - 1);
      }
    } else {
      return comb_Nm1_.next_combination();
    }
  }

  bool done() const { return done_; }

 private:
  unsigned int N_, smallest_elem_;
  Combination<k - 1> comb_Nm1_;
  bool done_;
};

template<>
class Combination<1> {
  Combination(unsigned int N) : smallest_elem_(0), N_(N) { }

  void reset(unsigned N) {
    smallest_elem_ = 0;
    N_ = N;
  }

  unsigned int get(unsigned int idx) const {
    assert(idx == 0);
    return smallest_elem_;
  }

  bool next_combination() {
    if (smallest_elem_ == N_ - 1) {
      return false;
    } else {
      ++smallest_elem_;
      return true;
    }
  }

  bool done() const { return smallest_elem_ == N_ - 1; }

 private:
  unsigned int smallest_elem_, N_;
};
*/

/**
 * Assumed creation operators and annihilation operators are time-ordered, respectively
 */
/*
template<unsigned int RANK>
int
pick_up_pair_imp(const std::vector<psi> &cdagg_ops,
                 const std::vector<psi> &c_ops,
                 double distance,
                 alps::random01 &rng,
                 std::vector<psi> &cdagg_ops_pick,
                 std::vector<psi> &c_ops_pick
) {
  cdagg_ops_pick.resize(0);
  c_ops_pick.resize(0);

  if (cdagg_ops.size() < RANK || c_ops.size() < RANK) {
    return 0;
  }

  int idx = 0, num_pair = -100, target = -1000;
  for (int path = 0; path < 2; ++path) {
    double max_cdagg, min_cdagg, max_c, min_c;
    for (Combination<RANK> comb1(cdagg_ops.size()); !comb1.done(); comb1.next_combination()) {
      max_cdagg = comb1.get(RANK - 1);
      min_cdagg = comb1.get(0);
      if (std::abs(max_cdagg - min_cdagg) > distance) {
        continue;
      }
      for (Combination<RANK> comb2(cdagg_ops.size()); !comb2.done(); comb2.next_combination()) {
        max_c = comb2.get(RANK - 1);
        min_c = comb2.get(0);
        if (std::abs(std::max(max_cdagg, max_c) - std::min(min_cdagg, min_c)) <= distance) {
          if (path == 1 && idx == target) {
            cdagg_ops_pick.resize(RANK);
            c_ops_pick.resize(RANK);
            for (int rank = 0; rank < RANK; ++rank) {
              cdagg_ops_pick[rank] = cdagg_ops[comb1.get(rank)];
              c_ops_pick[rank] = c_ops[comb2.get(rank)];
              return num_pair;
            }
          }
          ++idx;
        }
      }
    }
    if (path == 0) {
      num_pair = idx;
      target = static_cast<int>(rng() * num_pair);
    }
  }

  assert(false);
  return 0;
};
 */

/**
* Pick up a pair of creation and annihilation operators in a given time window and returns interators pointing to the picked-up operators
*
* @param pseudo-random-number generator
* @param c_operators the list of creation operators
* @param a_operators the list of annihilation operators
* @param flavor_ins the flavor of creation operators of the pairs
* @param flavor_rem the flavor of annihilation operators of the pairs
* @param t1 upper bound or lower bound of the time window
* @param t2 upper bound or lower bound of the time window
* @param distance cutoff for the mutual distance of a pair of creation and annihilation operators
* @param BETA inverse temperature
*/
/*
template<typename R, typename Iterator>
int
pick_up_pair(int rank,
             R &rng,
             Iterator cdagg_ops_first,
             Iterator cdagg_ops_end,
             Iterator c_ops_first,
             Iterator c_ops_end,
             const std::vector<bool> &flavor_mask,
             const std::vector<psi> &additional_cdagg_ops,
             const std::vector<psi> &additional_c_ops,
             double max_distance,
             std::vector<psi> &cdagg_ops_picked,
             std::vector<psi> &c_ops_picked
) {
  typedef std::vector<psi>::iterator Iterator2;

  struct inactive_flavor {
    bool operator()(const psi &op) const {
      return !flavor_mask[op.flavor()];
    }
  };

  //copy creation operators
  std::vector<psi> cdagg_ops;
  std::remove_copy_if(cdagg_ops_first, cdagg_ops_end, std::back_inserter(cdagg_ops), inactive_flavor());
  std::remove_copy_if(additional_cdagg_ops.begin(),
                      additional_cdagg_ops.end(),
                      std::back_inserter(cdagg_ops),
                      inactive_flavor());

  //copy annihilation operators
  std::vector<psi> c_ops;
  std::remove_copy_if(c_ops_first, c_ops_end, std::back_inserter(c_ops), inactive_flavor());
  std::remove_copy_if(additional_c_ops.begin(), additional_c_ops.end(), std::back_inserter(c_ops), inactive_flavor());

  if (rank == 1) {
    return pick_up_pair_imp<1>(cdagg_ops, c_ops, max_distance, rng, cdagg_ops_picked, c_ops_picked);
  } else if (rank == 2) {
    return pick_up_pair_imp<2>(cdagg_ops, c_ops, max_distance, rng, cdagg_ops_picked, c_ops_picked);
  } else if (rank == 3) {
    return pick_up_pair_imp<3>(cdagg_ops, c_ops, max_distance, rng, cdagg_ops_picked, c_ops_picked);
  } else if (rank == 4) {
    return pick_up_pair_imp<4>(cdagg_ops, c_ops, max_distance, rng, cdagg_ops_picked, c_ops_picked);
  } else if (rank == 5) {
    return pick_up_pair_imp<5>(cdagg_ops, c_ops, max_distance, rng, cdagg_ops_picked, c_ops_picked);
  } else {
    throw std::runtime_error("Not implemented pick_up_pair_imp");
  }
}
 */

template<typename SCALAR, typename EXTENDED_SCALAR, typename SLIDING_WINDOW>
class LocalUpdater {
 public:
  LocalUpdater() { }
  virtual ~LocalUpdater() { }

  /** Update the configuration */
  void update(
      alps::random01 &rng, double BETA,
      MonteCarloConfiguration<SCALAR> &mc_config,
      SLIDING_WINDOW &sliding_window
  );

  /** To be implemented in a derived class */
  virtual bool propose(
      alps::random01 &rng,
      MonteCarloConfiguration<SCALAR> &mc_config,
      const SLIDING_WINDOW &sliding_window
  ) = 0;

  /** Will be called on the exit of update() */
  virtual void call_back() { };

  /** updates parameters for Monte Carlo updates */
  virtual void update_parameters() { };

  /** fix parameters for Monte Carlo updates before measurement steps */
  virtual void finalize_learning() { }

  /** create measurement */
  virtual void create_measurement_acc_rate(alps::accumulators::accumulator_set &measurements) { }

  /** measure acceptance rate */
  virtual void measure_acc_rate(alps::accumulators::accumulator_set &measurements) { }

 protected:
  std::string name_;//name of this updator

  //the following variables will be set in virtual function propose();
  boost::optional<SCALAR> acceptance_rate_correction_;
  std::vector<psi> cdagg_ops_rem_; //hybrized with bath
  std::vector<psi> c_ops_rem_;     //hybrized with bath
  std::vector<psi> cdagg_ops_add_; //hybrized with bath
  std::vector<psi> c_ops_add_;     //hybrized with bath

  //some variables set on the exit of update()
  bool valid_move_generated_;
  bool accepted_;

 private:
  std::vector<psi> duplicate_check_work_;

  bool update_operators(MonteCarloConfiguration<SCALAR> &mc_config) {
    //erase should success. Otherwise, something went wrong in the proposal of the update.
    safe_erase(mc_config.operators, cdagg_ops_rem_.begin(), cdagg_ops_rem_.end());
    safe_erase(mc_config.operators, c_ops_rem_.begin(), c_ops_rem_.end());

    //insertion is more delicate. We may try to add two operators at the same tau accidentally.
    bool duplicate_found = false;
    {
      duplicate_check_work_.resize(0);
      std::copy(cdagg_ops_add_.begin(), cdagg_ops_add_.end(), std::back_inserter(duplicate_check_work_));
      std::copy(c_ops_add_.begin(), c_ops_add_.end(), std::back_inserter(duplicate_check_work_));
      std::sort(duplicate_check_work_.begin(), duplicate_check_work_.end());
      if (boost::adjacent_find(duplicate_check_work_) != duplicate_check_work_.end()) {
        duplicate_found = true;
      } else {
        for (std::vector<psi>::iterator it = duplicate_check_work_.begin(); it != duplicate_check_work_.end(); ++it) {
          if (mc_config.operators.find(*it) != mc_config.operators.end()) {
            duplicate_found = true;
            break;
          }
        }
      }
    }
    if (duplicate_found) {
      std::cout << "duplicate found " << std::endl;
      safe_insert(mc_config.operators, cdagg_ops_rem_.begin(), cdagg_ops_rem_.end());
      safe_insert(mc_config.operators, c_ops_rem_.begin(), c_ops_rem_.end());
      return false;
    }

    safe_insert(mc_config.operators, cdagg_ops_add_.begin(), cdagg_ops_add_.end());
    safe_insert(mc_config.operators, c_ops_add_.begin(), c_ops_add_.end());
    return true;
  }

  void revert_operators(MonteCarloConfiguration<SCALAR> &mc_config) {
    safe_erase(mc_config.operators, cdagg_ops_add_.begin(), cdagg_ops_add_.end());
    safe_erase(mc_config.operators, c_ops_add_.begin(), c_ops_add_.end());
    safe_insert(mc_config.operators, cdagg_ops_rem_.begin(), cdagg_ops_rem_.end());
    safe_insert(mc_config.operators, c_ops_rem_.begin(), c_ops_rem_.end());
  }

  void finalize_update() {
    call_back();

    acceptance_rate_correction_ = boost::none;
    cdagg_ops_rem_.resize(0);
    c_ops_rem_.resize(0);
    cdagg_ops_add_.resize(0);
    c_ops_add_.resize(0);
    valid_move_generated_ = false;
    accepted_ = false;
  }

  std::vector<EXTENDED_REAL> trace_bound;//must be resized
};

void range_check(std::vector<psi> &ops, double tau_low, double tau_high) {
  for (std::vector<psi>::iterator it = ops.begin(); it != ops.end(); ++it) {
    if (tau_low > it->time().time() || tau_high < it->time().time()) {
      throw std::runtime_error("Something went wrong: try to update operators outside the range");
    }
  }
}

template<typename SCALAR, typename EXTENDED_SCALAR, typename SLIDING_WINDOW>
void LocalUpdater<SCALAR, EXTENDED_SCALAR, SLIDING_WINDOW>::update(
    alps::random01 &rng, double BETA,
    MonteCarloConfiguration<SCALAR> &mc_config,
    SLIDING_WINDOW &sliding_window
) {
  accepted_ = false;

  valid_move_generated_ = propose(
      rng, mc_config, sliding_window
  );

  if (!valid_move_generated_) {
    finalize_update();
    return;
  } else {
    assert(acceptance_rate_correction_);
  }

  //make sure all operators to be updated in the window
  range_check(cdagg_ops_rem_, sliding_window.get_tau_low(), sliding_window.get_tau_high());
  range_check(c_ops_rem_, sliding_window.get_tau_low(), sliding_window.get_tau_high());
  range_check(cdagg_ops_add_, sliding_window.get_tau_low(), sliding_window.get_tau_high());
  range_check(c_ops_add_, sliding_window.get_tau_low(), sliding_window.get_tau_high());

  //update operators
  bool update_success = update_operators(mc_config);
  if (!update_success) {
    finalize_update();
    return;
  }

  //compute the upper bound of trace
  trace_bound.resize(sliding_window.get_num_brakets());
  const EXTENDED_REAL trace_bound_sum = sliding_window.compute_trace_bound(mc_config.operators, trace_bound);
  if (trace_bound_sum == 0.0) {
    revert_operators(mc_config);
    finalize_update();
    return;
  }

  //compute the determinant ratio
  const SCALAR det_rat =
      mc_config.M.try_update(
          cdagg_ops_rem_.begin(), cdagg_ops_rem_.end(),
          c_ops_rem_.begin(), c_ops_rem_.end(),
          cdagg_ops_add_.begin(), cdagg_ops_add_.end(),
          c_ops_add_.begin(), c_ops_add_.end()
      );

  //flip a coin for lazy evaluation of trace
  const double r_th = rng();

  bool accepted = false;
  EXTENDED_SCALAR trace_new;
  SCALAR prob;
  if (det_rat != 0.0) {
    const SCALAR rest = (*acceptance_rate_correction_) * det_rat;
    const EXTENDED_REAL trace_cutoff = myabs(r_th * mc_config.trace / rest);
    boost::tie(accepted, trace_new) = sliding_window.lazy_eval_trace(mc_config.operators, trace_cutoff, trace_bound);
    prob = rest * convert_to_scalar(static_cast<EXTENDED_SCALAR>(trace_new / mc_config.trace));
    assert(myabs(trace_new) < myabs(trace_bound_sum) * 1.01);
    assert(accepted == std::abs(prob) > r_th);
  } else {
    trace_new = 0.0;
    prob = 0.0;
  }

  if (accepted) { // move accepted
    mc_config.M.perform_update();
    const int perm_new = compute_permutation_sign(mc_config);
    mc_config.sign *= (1. * perm_new / mc_config.perm_sign) * mysign(prob);
    assert(!my_isnan(mc_config.sign));
    mc_config.perm_sign = perm_new;
    mc_config.trace = trace_new;
    accepted_ = true;
  } else { // rejected
    mc_config.M.reject_update();
    revert_operators(mc_config);
  }
  mc_config.check_nan();

  finalize_update();
};

/**
 * Update creation and annihilation operators hybridized with the bath
 * Do not update the worm
 */
template<typename SCALAR, typename EXTENDED_SCALAR, typename SLIDING_WINDOW>
class InsertionRemovalUpdater: public LocalUpdater<SCALAR, EXTENDED_SCALAR, SLIDING_WINDOW> {
 public:
  typedef LocalUpdater<SCALAR, EXTENDED_SCALAR, SLIDING_WINDOW> BaseType;
  InsertionRemovalUpdater(int update_rank, int num_flavors)
      : update_rank_(update_rank),
        num_flavors_(num_flavors),
        tau_low_(-1.0),
        tau_high_(-1.0) { }

  virtual bool propose(
      alps::random01 &rng,
      MonteCarloConfiguration<SCALAR> &mc_config,
      const SLIDING_WINDOW &sliding_window
  ) {
    namespace bll = boost::lambda;

    tau_low_ = sliding_window.get_tau_low();
    tau_high_ = sliding_window.get_tau_high();

    const int num_blocks = mc_config.M.num_blocks();
    num_cdagg_ops_in_range_.resize(num_blocks);
    num_c_ops_in_range_.resize(num_blocks);
    cdagg_ops_range_.resize(num_blocks);
    c_ops_range_.resize(num_blocks);
    for (int ib = 0; ib < num_blocks; ++ib) {
      cdagg_ops_range_[ib] = mc_config.M.get_cdagg_ops_set(ib).range(
          tau_low_ <= bll::_1, bll::_1 <= tau_high_
      );
      c_ops_range_[ib] = mc_config.M.get_c_ops_set(ib).range(
          tau_low_ <= bll::_1, bll::_1 <= tau_high_
      );
      num_cdagg_ops_in_range_[ib] = std::distance(cdagg_ops_range_[ib].first, cdagg_ops_range_[ib].second);
      num_c_ops_in_range_[ib] = std::distance(c_ops_range_[ib].first, c_ops_range_[ib].second);
    }

    if (rng() < 0.5) {
      return propose_insertion(rng, mc_config);
    } else {
      return propose_removal(rng, mc_config);
    }
  }

 protected:
  typedef operator_container_t::iterator it_t;

  const int num_flavors_;
  double tau_low_, tau_high_; //, max_distance_;

  std::vector<int> num_cdagg_ops_in_range_, num_c_ops_in_range_;
  std::vector<std::pair<it_t, it_t> > cdagg_ops_range_, c_ops_range_;

  /** 1 for two-operator update, 2 for four-operator update, ..., N for 2N-operator update*/
  const int update_rank_;

/**
 * Propose insertion update
 */
  bool propose_insertion(alps::random01 &rng,
                         MonteCarloConfiguration<SCALAR> &mc_config);

/**
 * Propose removal update
 */
  bool propose_removal(alps::random01 &rng,
                       MonteCarloConfiguration<SCALAR> &mc_config);
};

template<typename SCALAR, typename EXTENDED_SCALAR, typename SLIDING_WINDOW>
bool InsertionRemovalUpdater<SCALAR, EXTENDED_SCALAR, SLIDING_WINDOW>::propose_insertion(
    alps::random01 &rng,
    MonteCarloConfiguration<SCALAR> &mc_config) {
  namespace bll = boost::lambda;
  typedef operator_container_t::iterator it_t;

  const int num_blocks = mc_config.M.num_blocks();
  std::vector<int> num_new_pairs(num_blocks, 0);
  for (int iop = 0; iop < update_rank_; ++iop) {
    const int block = static_cast<int>(rng() * num_blocks);
    num_new_pairs[block] += 1;
    BaseType::cdagg_ops_add_.push_back(
        psi(open_random(rng, tau_low_, tau_high_),
            CREATION_OP,
            pick(mc_config.M.flavors(block), rng)
        )
    );
    BaseType::c_ops_add_.push_back(
        psi(open_random(rng, tau_low_, tau_high_),
            ANNIHILATION_OP,
            pick(mc_config.M.flavors(block), rng)
        )
    );
  }

  BaseType::acceptance_rate_correction_ = std::pow(tau_high_ - tau_low_, 2. * update_rank_);
  for (int ib = 0; ib < num_blocks; ++ib) {
    if (num_new_pairs[ib] == 0) {
      continue;
    }
    BaseType::acceptance_rate_correction_ =
        (*BaseType::acceptance_rate_correction_) *
            boost::math::factorial<double>(1. * num_new_pairs[ib])
            * (1. * mc_config.M.num_flavors(ib))
            * (1. * mc_config.M.num_flavors(ib));
    BaseType::acceptance_rate_correction_ =
        (*BaseType::acceptance_rate_correction_) /
            boost::math::binomial_coefficient<double>(
                num_cdagg_ops_in_range_[ib] + num_new_pairs[ib],
                num_new_pairs[ib]
            );
    BaseType::acceptance_rate_correction_ =
        (*BaseType::acceptance_rate_correction_) /
            boost::math::binomial_coefficient<double>(
                num_c_ops_in_range_[ib] + num_new_pairs[ib],
                num_new_pairs[ib]
            );
  }

  return true;
};

template<typename SCALAR, typename EXTENDED_SCALAR, typename SLIDING_WINDOW>
bool InsertionRemovalUpdater<SCALAR, EXTENDED_SCALAR, SLIDING_WINDOW>::propose_removal(
    alps::random01 &rng,
    MonteCarloConfiguration<SCALAR> &mc_config) {

  const int num_blocks = mc_config.M.num_blocks();
  std::vector<int> num_pairs_rem(num_blocks);
  for (int iop = 0; iop < update_rank_; ++iop) {
    num_pairs_rem[static_cast<int>(rng() * num_blocks)] += 1;
  }

  //check if there are enough operators to be removed
  for (int ib = 0; ib < num_blocks; ++ib) {
    if (num_cdagg_ops_in_range_[ib] < num_pairs_rem[ib] || num_c_ops_in_range_[ib] < num_pairs_rem[ib]) {
      return false;
    }
  }

  //pick up operators
  for (int ib = 0; ib < num_blocks; ++ib) {
    const std::vector<int> &idx_c = pickup_a_few_numbers(num_cdagg_ops_in_range_[ib], num_pairs_rem[ib], rng);
    const std::vector<int> &idx_a = pickup_a_few_numbers(num_c_ops_in_range_[ib], num_pairs_rem[ib], rng);
    for (int iop = 0; iop < num_pairs_rem[ib]; ++iop) {
      {
        it_t it = cdagg_ops_range_[ib].first;
        std::advance(it, idx_c[iop]);
        BaseType::cdagg_ops_rem_.push_back(*it);
      }
      {
        it_t it = c_ops_range_[ib].first;
        std::advance(it, idx_a[iop]);
        BaseType::c_ops_rem_.push_back(*it);
      }
    }
  }

  BaseType::acceptance_rate_correction_ = 1.0 / std::pow(tau_high_ - tau_low_, 2. * update_rank_);
  for (int ib = 0; ib < num_blocks; ++ib) {
    if (num_pairs_rem[ib] == 0) {
      continue;
    }
    BaseType::acceptance_rate_correction_ =
        (*BaseType::acceptance_rate_correction_) /
            (
                boost::math::factorial<double>(1. * num_pairs_rem[ib])
                    * (1. * mc_config.M.num_flavors(ib))
                    * (1. * mc_config.M.num_flavors(ib))
            );
    BaseType::acceptance_rate_correction_ =
        (*BaseType::acceptance_rate_correction_) *
            boost::math::binomial_coefficient<double>(num_cdagg_ops_in_range_[ib], num_pairs_rem[ib]);
    BaseType::acceptance_rate_correction_ =
        (*BaseType::acceptance_rate_correction_) *
            boost::math::binomial_coefficient<double>(num_c_ops_in_range_[ib], num_pairs_rem[ib]);
  }

  return true;
};

/**
 * Update creation and annihilation operators hybridized with the bath (the same flavor)
 * Do not update the worm
 */
template<typename SCALAR, typename EXTENDED_SCALAR, typename SLIDING_WINDOW>
class InsertionRemovalDiagonalUpdater: public LocalUpdater<SCALAR, EXTENDED_SCALAR, SLIDING_WINDOW> {
 public:
  typedef LocalUpdater<SCALAR, EXTENDED_SCALAR, SLIDING_WINDOW> BaseType;
  InsertionRemovalDiagonalUpdater(int update_rank, int num_flavors, double beta, int num_bins)
      : update_rank_(update_rank),
        num_flavors_(num_flavors),
        beta_(beta),
        tau_low_(-1.0),
        tau_high_(-1.0),
        acc_rate_(num_bins, 0.5 * beta, num_flavors, 0.5 * beta) { }

  virtual bool propose(
      alps::random01 &rng,
      MonteCarloConfiguration<SCALAR> &mc_config,
      const SLIDING_WINDOW &sliding_window
  ) {
    namespace bll = boost::lambda;
    typedef operator_container_t::iterator IteratorType;

    tau_low_ = sliding_window.get_tau_low();
    tau_high_ = sliding_window.get_tau_high();

    flavor_ = static_cast<int>(rng() * num_flavors_);
    const int block = mc_config.M.block_belonging_to(flavor_);

    //count creation operators
    {
      std::pair<IteratorType, IteratorType> ops_range = mc_config.M.get_cdagg_ops_set(block).range(
          tau_low_ <= bll::_1, bll::_1 <= tau_high_
      );
      cdagg_ops_in_range_.resize(0);
      for (IteratorType it = ops_range.first; it != ops_range.second; ++it) {
        if (it->flavor() == flavor_) {
          cdagg_ops_in_range_.push_back(*it);
        }
      }
    }

    //count annihilation operators
    {
      c_ops_in_range_.resize(0);
      std::pair<IteratorType, IteratorType> ops_range = mc_config.M.get_c_ops_set(block).range(
          tau_low_ <= bll::_1, bll::_1 <= tau_high_
      );
      for (IteratorType it = ops_range.first; it != ops_range.second; ++it) {
        if (it->flavor() == flavor_) {
          c_ops_in_range_.push_back(*it);
        }
      }
    }

    if (rng() < 0.5) {
      for (int iop = 0; iop < update_rank_; ++iop) {
        BaseType::cdagg_ops_add_.push_back(
            psi(open_random(rng, tau_low_, tau_high_),
                CREATION_OP,
                flavor_)
        );
        BaseType::c_ops_add_.push_back(
            psi(open_random(rng, tau_low_, tau_high_),
                ANNIHILATION_OP,
                flavor_)
        );
      }
      BaseType::acceptance_rate_correction_ =
          std::pow(tau_high_ - tau_low_, 2. * update_rank_) / (
              boost::math::binomial_coefficient<double>(cdagg_ops_in_range_.size() + update_rank_, update_rank_)
                  * boost::math::binomial_coefficient<double>(c_ops_in_range_.size() + update_rank_, update_rank_)
                  * boost::math::factorial<double>(update_rank_)
          );

      std::sort(BaseType::cdagg_ops_add_.begin(), BaseType::cdagg_ops_add_.end());
      std::sort(BaseType::c_ops_add_.begin(), BaseType::c_ops_add_.end());
      distance_ = std::max(BaseType::cdagg_ops_add_.back().time().time(), BaseType::c_ops_add_.back().time().time())
          - std::min(BaseType::cdagg_ops_add_[0].time().time(), BaseType::c_ops_add_[0].time().time());
      return true;
    } else {
      if (cdagg_ops_in_range_.size() < update_rank_ || c_ops_in_range_.size() < update_rank_) {
        return false;
      }
      const std::vector<int> &idx_c = pickup_a_few_numbers(cdagg_ops_in_range_.size(), update_rank_, rng);
      const std::vector<int> &idx_a = pickup_a_few_numbers(c_ops_in_range_.size(), update_rank_, rng);
      for (int iop = 0; iop < update_rank_; ++iop) {
        BaseType::cdagg_ops_rem_.push_back(
            cdagg_ops_in_range_[idx_c[iop]]
        );
        BaseType::c_ops_rem_.push_back(
            c_ops_in_range_[idx_a[iop]]
        );
      }
      BaseType::acceptance_rate_correction_ =
          (boost::math::binomial_coefficient<double>(cdagg_ops_in_range_.size(), update_rank_)
              * boost::math::binomial_coefficient<double>(c_ops_in_range_.size(), update_rank_)
              * boost::math::factorial<double>(update_rank_)
          )
              / std::pow(tau_high_ - tau_low_, 2. * update_rank_);
      std::sort(BaseType::cdagg_ops_rem_.begin(), BaseType::cdagg_ops_rem_.end());
      std::sort(BaseType::c_ops_rem_.begin(), BaseType::c_ops_rem_.end());
      distance_ = std::max(BaseType::cdagg_ops_rem_.back().time().time(), BaseType::c_ops_rem_.back().time().time())
          - std::min(BaseType::cdagg_ops_rem_[0].time().time(), BaseType::c_ops_rem_[0].time().time());
      return true;
    }
  }

  virtual void call_back() {
    if (!BaseType::valid_move_generated_) {
      return;
    }

    if (BaseType::accepted_) {
      acc_rate_.add_sample(std::min(distance_, beta_ - distance_), 1.0, flavor_);
    } else {
      acc_rate_.add_sample(std::min(distance_, beta_ - distance_), 0.0, flavor_);
    }
  };

  virtual void finalize_learning() {
    acc_rate_.reset();
  }

  virtual void create_measurement_acc_rate(alps::accumulators::accumulator_set &measurements) {
    measurements << alps::accumulators::NoBinningAccumulator<std::vector<double> >(
        "InsertionRemovalDiagonalRank"
            + boost::lexical_cast<std::string>(update_rank_)
            + "_attempted");

    measurements << alps::accumulators::NoBinningAccumulator<std::vector<double> >(
        "InsertionRemovalDiagonalRank"
            + boost::lexical_cast<std::string>(update_rank_)
            + "_accepted");
  }

  virtual void measure_acc_rate(alps::accumulators::accumulator_set &measurements) {
    measurements[
        "InsertionRemovalDiagonalRank"
            + boost::lexical_cast<std::string>(update_rank_)
            + "_attempted"
    ] << to_std_vector(acc_rate_.get_counter());
    measurements["InsertionRemovalDiagonalRank"
        + boost::lexical_cast<std::string>(update_rank_)
        + "_accepted"] << to_std_vector(acc_rate_.get_sumval());
    acc_rate_.reset();
  }

 private:
  const int num_flavors_;
  const double beta_;
  double tau_low_, tau_high_;
  int flavor_;

  std::vector<psi> cdagg_ops_in_range_, c_ops_in_range_;

  /** 1 for two-operator update, 2 for four-operator update, ..., N for 2N-operator update*/
  const int update_rank_;

  scalar_histogram_flavors acc_rate_;
  double distance_;
};

/**
 * Update creation and annihilation operators hybridized with the bath (the same flavor)
 * Do not update the worm
 */
template<typename SCALAR, typename EXTENDED_SCALAR, typename SLIDING_WINDOW>
class SingleOperatorShiftUpdater: public LocalUpdater<SCALAR, EXTENDED_SCALAR, SLIDING_WINDOW> {
 public:
  typedef LocalUpdater<SCALAR, EXTENDED_SCALAR, SLIDING_WINDOW> BaseType;
  SingleOperatorShiftUpdater(double beta, int num_flavors, int num_bins) :
      num_flavors_(num_flavors),
      max_distance_(num_flavors, 0.5 * beta),
      acc_rate_(num_bins, 0.5 * beta, num_flavors, 0.5 * beta) { }

  virtual bool propose(
      alps::random01 &rng,
      MonteCarloConfiguration<SCALAR> &mc_config,
      const SLIDING_WINDOW &sliding_window
  ) {
    namespace bll = boost::lambda;
    typedef operator_container_t::iterator IteratorType;

    double tau_low = sliding_window.get_tau_low();
    double tau_high = sliding_window.get_tau_high();

    std::pair<IteratorType, IteratorType> cdagg_ops_range =
        mc_config.operators.range(tau_low <= bll::_1, bll::_1 <= tau_high);
    const int num_cdagg_ops = std::distance(cdagg_ops_range.first, cdagg_ops_range.second);

    std::pair<IteratorType, IteratorType> c_ops_range =
        mc_config.operators.range(tau_low <= bll::_1, bll::_1 <= tau_high);
    const int num_c_ops = std::distance(c_ops_range.first, c_ops_range.second);

    if (num_cdagg_ops + num_c_ops == 0) {
      return false;
    }

    const int idx = static_cast<int>(rng() * (num_cdagg_ops + num_c_ops));

    if (idx < num_cdagg_ops) {
      IteratorType it = cdagg_ops_range.first;
      std::advance(it, idx);
      flavor_ = it->flavor();
      const int new_flavor = rng() < 0.5 ? it->flavor() : gen_new_flavor(mc_config, flavor_, rng);
      const double new_time = (2 * rng() - 1.0) * max_distance_[flavor_] + it->time().time();
      if (new_time < tau_low || new_time > tau_high) { return false; }
      distance_ = std::abs(it->time().time() - new_time);
      BaseType::cdagg_ops_rem_.push_back(*it);
      BaseType::cdagg_ops_add_.push_back(
          psi(new_time, CREATION_OP, new_flavor)
      );
    } else {
      IteratorType it = c_ops_range.first;
      std::advance(it, idx - num_cdagg_ops);
      flavor_ = it->flavor();
      const int new_flavor = rng() < 0.5 ? it->flavor() : gen_new_flavor(mc_config, flavor_, rng);
      const double new_time = (2 * rng() - 1.0) * max_distance_[flavor_] + it->time().time();
      if (new_time < tau_low || new_time > tau_high) { return false; }
      distance_ = std::abs(it->time().time() - new_time);
      BaseType::c_ops_rem_.push_back(*it);
      BaseType::c_ops_add_.push_back(
          psi(new_time, ANNIHILATION_OP, new_flavor)
      );
    }
    BaseType::acceptance_rate_correction_ = 1.0;
    return true;
  }


  virtual void call_back() {
    if (!BaseType::valid_move_generated_) {
      return;
    }

    if (BaseType::accepted_) {
      acc_rate_.add_sample(distance_, 1.0, flavor_);
    } else {
      acc_rate_.add_sample(distance_, 0.0, flavor_);
    }
  };

  virtual void update_parameters() {
    for (int flavor = 0; flavor < num_flavors_; ++flavor) {
      max_distance_[flavor] = acc_rate_.update_cutoff(1E-2, 1.05);
    }
  }

  virtual void finalize_learning() {
    acc_rate_.reset();
  }

  virtual void create_measurement_acc_rate(alps::accumulators::accumulator_set &measurements) {
    measurements << alps::accumulators::NoBinningAccumulator<std::vector<double> >("Shift_attempted");
    measurements << alps::accumulators::NoBinningAccumulator<std::vector<double> >("Shift_accepted");
  }

  virtual void measure_acc_rate(alps::accumulators::accumulator_set &measurements) {
    measurements["Shift_attempted"] << to_std_vector(acc_rate_.get_counter());
    measurements["Shift_accepted"] << to_std_vector(acc_rate_.get_sumval());
    acc_rate_.reset();
  }

 private:
  int num_flavors_;
  scalar_histogram_flavors acc_rate_;

  std::vector<double> max_distance_;
  double distance_;
  int flavor_;

  static int gen_new_flavor(const MonteCarloConfiguration<SCALAR> &mc_config, int old_flavor, alps::random01 &rng) {
    const int block = mc_config.M.block_belonging_to(old_flavor);
    return pick(mc_config.M.flavors(block), rng);
  }
};

template<typename SCALAR>
SCALAR compute_det_rat(
    const std::vector<SCALAR> &det_vec_new,
    const std::vector<SCALAR> &det_vec_old,
    double eps = 1E-30) {
  const int num_loop = std::max(det_vec_new.size(), det_vec_old.size());

  const double max_abs_elem = std::abs(*std::max_element(det_vec_new.begin(), det_vec_new.end(), AbsLessor<SCALAR>()));

  SCALAR det_rat = 1.0;
  for (int i = 0; i < num_loop; ++i) {
    if (i < det_vec_new.size() && std::abs(det_vec_new[i] / max_abs_elem) > eps) {
      det_rat *= det_vec_new[i];
    }
    if (i < det_vec_old.size()) {
      det_rat /= det_vec_old[i];
    }
  }
  return det_rat;
}

template<typename Scalar, typename M>
std::vector<Scalar>
lu_product(const M &matrix) {
  if (matrix.rows() == 0) {
    return std::vector<Scalar>();
  }
  Eigen::FullPivLU<Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> > lu(matrix);
  const int size1 = lu.rows();
  std::vector<Scalar> results(size1);
  for (int i = 0; i < size1; ++i) {
    results[i] = lu.matrixLU()(i, i);
  }
  results[0] *= lu.permutationP().determinant() * lu.permutationQ().determinant();
  return results;
};

template<typename SCALAR, typename GreensFunction, typename DetMatType>
SCALAR compute_det_rat(const std::vector<psi> &creation_operators,
                       const std::vector<psi> &annihilation_operators,
                       std::vector<SCALAR> &det_vec_old,
                       DetMatType &M,
                       std::vector<SCALAR> &det_vec_new
) {
  typedef alps::fastupdate::ResizableMatrix<SCALAR> M_TYPE;
  //std::vector<Eigen::Matrix<SCALAR,Eigen::Dynamic,Eigen::Dynamic> > M_new(M.num_blocks());
  std::vector<std::vector<psi> > cdagg_ops(M.num_blocks()), c_ops(M.num_blocks());

  for (std::vector<psi>::const_iterator it = creation_operators.begin(); it != creation_operators.end(); ++it) {
    cdagg_ops[M.block_belonging_to(it->flavor())].push_back(*it);
  }
  for (std::vector<psi>::const_iterator it = annihilation_operators.begin(); it != annihilation_operators.end(); ++it) {
    c_ops[M.block_belonging_to(it->flavor())].push_back(*it);
  }


  //compute determinant as a product
  boost::shared_ptr<GreensFunction> p_gf = M.get_greens_function();
  std::vector<OperatorTime> cdagg_times, c_times;
  det_vec_new.resize(0);
  det_vec_new.reserve(creation_operators.size());
  for (int ib = 0; ib < M.num_blocks(); ++ib) {
    const int mat_size = cdagg_ops[ib].size();
    assert(cdagg_ops[ib].size() == c_ops[ib].size());
    if (mat_size == 0) {
      continue;
    }
    Eigen::Matrix<SCALAR, Eigen::Dynamic, Eigen::Dynamic> M_new(mat_size, mat_size);
    for (int col = 0; col < mat_size; ++col) {
      for (int row = 0; row < mat_size; ++row) {
        M_new(row, col) = p_gf->operator()(c_ops[ib][row], cdagg_ops[ib][col]);
      }
    }
    const std::vector<SCALAR> &vec_tmp = lu_product<SCALAR>(M_new);
    std::copy(vec_tmp.begin(), vec_tmp.end(), std::back_inserter(det_vec_new));

    for (int col = 0; col < mat_size; ++col) {
      cdagg_times.push_back(cdagg_ops[ib][col].time());
    }
    for (int row = 0; row < mat_size; ++row) {
      c_times.push_back(c_ops[ib][row].time());
    }
  }

  if (det_vec_new.size() == 0) {
    return 0.0;
  }

  //compute determinant ratio
  std::sort(det_vec_old.begin(), det_vec_old.end(), AbsGreater<SCALAR>());
  std::sort(det_vec_new.begin(), det_vec_new.end(), AbsGreater<SCALAR>());
  const SCALAR det_rat = compute_det_rat(det_vec_new, det_vec_old);

  //compute permulation sign from exchange of row and col
  const int perm_sign_block = alps::fastupdate::comb_sort(cdagg_times.begin(), cdagg_times.end(), OperatorTimeLessor())
      * alps::fastupdate::comb_sort(c_times.begin(), c_times.end(), OperatorTimeLessor());

  det_vec_new[0] *= 1. * perm_sign_block;
  return (1. * perm_sign_block) * det_rat;
}

template<typename SCALAR, typename EXTENDED_SCALAR, typename R, typename SLIDING_WINDOW, typename OperatorTransformer>
bool
global_update(R &rng,
              double BETA,
              MonteCarloConfiguration<SCALAR> &mc_config,
              std::vector<SCALAR> &det_vec,
              SLIDING_WINDOW &sliding_window,
              int num_flavors,
              OperatorTransformer transformer,
              int Nwin
) {
  assert(sliding_window.get_tau_low() == 0);
  assert(sliding_window.get_tau_high() == BETA);
  const int pert_order = mc_config.pert_order();
  if (pert_order == 0) {
    return true;
  }

  //compute new trace (we use sliding window to avoid overflow/underflow).
  operator_container_t operators_new;
  for (operator_container_t::iterator it = mc_config.operators.begin();
       it != mc_config.operators.end();
       ++it) {
    operators_new.insert(transformer(*it));
  }
  sliding_window.set_window_size(1, mc_config.operators, 0, ITIME_LEFT);
  sliding_window.set_window_size(Nwin, operators_new, 0, ITIME_LEFT);

  std::vector<EXTENDED_REAL> trace_bound(sliding_window.get_num_brakets());
  sliding_window.compute_trace_bound(operators_new, trace_bound);

  std::pair<bool, EXTENDED_SCALAR> r = sliding_window.lazy_eval_trace(operators_new, EXTENDED_REAL(0.0), trace_bound);
  const EXTENDED_SCALAR trace_new = r.second;

  sliding_window.set_window_size(1, mc_config.operators, 0, ITIME_LEFT);
  if (trace_new == EXTENDED_SCALAR(0.0)) {
    return false;
  }

  //compute new operators
  std::vector<psi> creation_operators_new, annihilation_operators_new;
  std::transform(
      mc_config.M.get_cdagg_ops().begin(), mc_config.M.get_cdagg_ops().end(),
      std::back_inserter(creation_operators_new),
      transformer);
  std::transform(
      mc_config.M.get_c_ops().begin(), mc_config.M.get_c_ops().end(),
      std::back_inserter(annihilation_operators_new),
      transformer);

  //compute determinant ratio
  std::vector<SCALAR> det_vec_new;
  const SCALAR det_rat = compute_det_rat<SCALAR, HybridizationFunction<SCALAR> >(
      creation_operators_new, annihilation_operators_new,
      det_vec, mc_config.M, det_vec_new);

  const SCALAR prob =
      convert_to_scalar(
          EXTENDED_SCALAR(
              EXTENDED_SCALAR(det_rat) *
                  EXTENDED_SCALAR(trace_new / mc_config.trace)
          )
      );

  if (rng() < std::abs(prob)) {
    std::vector<std::pair<psi, psi> > operator_pairs(pert_order);
    for (int iop = 0; iop < pert_order; ++iop) {
      operator_pairs[iop] = std::make_pair(creation_operators_new[iop], annihilation_operators_new[iop]);
    }
    typedef typename MonteCarloConfiguration<SCALAR>::DeterminantMatrixType DeterminantMatrixType;
    DeterminantMatrixType M_new(
        mc_config.M.get_greens_function(),
        operator_pairs.begin(),
        operator_pairs.end()
    );

    mc_config.trace = trace_new;
    std::swap(mc_config.operators, operators_new);
    std::swap(mc_config.M, M_new);
    const int perm_sign_new = compute_permutation_sign(mc_config);
    mc_config.sign *= (1. * perm_sign_new / mc_config.perm_sign) * prob / std::abs(prob);
    mc_config.perm_sign = perm_sign_new;
    std::swap(det_vec, det_vec_new);
    mc_config.sanity_check(sliding_window);
    return true;
  } else {
    return false;
  }
}
