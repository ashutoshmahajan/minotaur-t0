//
//     MINOTAUR -- It's only 1/2 bull
//
//     (C)opyright 2008 - 2012 The MINOTAUR Team.
//

/**
 * \file Transformer.h
 * \brief Declare base class for transforming problems.
 * \author Ashutosh Mahajan, Argonne National Laboratory
 */

#ifndef MINOTAURTRANSFORMER_H
#define MINOTAURTRANSFORMER_H

#include "Types.h"
#include "OpCode.h"

namespace Minotaur {
class CxQuadHandler;
class CxUnivarHandler;
class CGraph;
class CNode;
class Environment;
class LinearHandler;
class Problem;
class Solution;
class YEqUCGs;
class YEqLFs;
class YEqVars;
typedef boost::shared_ptr<CxQuadHandler> CxQuadHandlerPtr;
typedef boost::shared_ptr<CxUnivarHandler> CxUnivarHandlerPtr;
typedef boost::shared_ptr<CGraph> CGraphPtr;
typedef boost::shared_ptr<Environment> EnvPtr;
typedef boost::shared_ptr<LinearHandler> LinearHandlerPtr;
typedef boost::shared_ptr<Problem> ProblemPtr;
typedef boost::shared_ptr<Solution> SolutionPtr;
typedef boost::shared_ptr<const Solution> ConstSolutionPtr;


/**
 * \brief Base class for reformulating a problem so that handlers can be
 * applied to it.
 *
 * A transformer will create a new problem equivalent to a given problem by
 * spliting constraints, adding new variables etc. The end result is a
 * problem whose each constraint can be handled by a specific handler. This
 * class has some pure virtual methods that must be implemented by a derived
 * class. Other commonly used functions are implemented.
 */
class Transformer {
public:

  /// Default Constructor.
  Transformer();

  /// Constructor.
  Transformer(EnvPtr env);

  /// Destroy.
  virtual ~Transformer();

  /**
   * \brief Translate the solution of reformulated problem into that of
   * original problem.
   *
   * \param [in] sol Solution of the reformulated problem.
   * \param [out] err Zero if no error is encountered, nonzero otherwise.
   * \return Solution of original problem.
   */
  virtual SolutionPtr getSolOrig(ConstSolutionPtr sol, Int &err) = 0;

  /**
   * \brief Translate the solution of originial problem into that of
   * reformulated problem.
   *
   * \param [in] sol Solution of the original problem.
   * \param [out] err Zero if no error is encountered, nonzero otherwise.
   * \return Solution of the reformulated problem.
   */
  virtual SolutionPtr getSolTrans(ConstSolutionPtr sol, Int &err) = 0;

  /**
   * \brief Perform the reformulation, and assign handlers.  
   *
   * \param [in] oldp The original problem that we want to reformulate.
   * \param [out] newp The new, reformulated problem.
   * \param [out] handlers A vector of handlers used to reformulate the
   * problem.
   * \param [out] status Zero if reformulated successfully. nonzero otherwise.
   */
  virtual void reformulate(ConstProblemPtr oldp, ProblemPtr &newp, 
                           HandlerVector &handlers, Int &status) = 0;

  /**
   * Takes Problem pointed to in p, and turns it into one with a linear
   * objective by adding a new variable if necessary.
   * \param [in] newp The new, reformulated problem.
   */
  virtual void makeObjectiveLinear(ProblemPtr newp);

protected:
  /// The pointer to environment.
  EnvPtr env_;

  /// Handler for quadratic terms
  CxQuadHandlerPtr qHandler_;

  /// Handler for univariate constraints.
  CxUnivarHandlerPtr uHandler_;

  /// Handler for linear constraints and variables.
  LinearHandlerPtr lHandler_;

  /// Logger
  LoggerPtr logger_;

  /**
   * \brief Storage for auxiliary variables defined by relations of the form
   * \f$y_i = x_j + d\f$.
   */
  YEqVars *yVars_;

  /**
   * \brief Storage for auxiliary variables defined by relations of the form
   * \f$y_i = c^Tx + d\f$.
   */
  YEqLFs *yLfs_;

  /**
   * \brief Storage for auxiliary variables defined by relations of the form
   * \f$y_i = f(x_j)\f$.
   */
  YEqUCGs *yUniExprs_;

  /// Tolerance for checking if a value is zero.
  const Double zTol_;

  /**
   * \brief Check if all constraints in a problem have been assigned to a
   * handler.
   *
   * \param[in] p Problem whose constraints need to be checked.
   * \returns True if all constraints have been assigned. False otherwise.
   */
  Bool allConsAssigned_(ProblemPtr p, HandlerVector &handlers);

  /**
   * \brief Assign an appropriate handler to a nonlinear constraint of the
   * form \f$y_i = f(x)\f$.
   *
   * \param[in] cg A nonlinear function which is be replaced by the auxiliary
   * variable.
   * \param[in] c The nonlinear constraint \f$y_i = f(x)\f$ that is being
   * assigned to. 
   */
  void assignHandler_(CGraphPtr cg, ConstraintPtr c);

  /**
   * \brief Delete unused handlers.
   *
   * \param [in/out] handlers. Contains pointers to each handler. Unused ones
   * are removed from the vector.
   */
  void clearUnusedHandlers_(HandlerVector &handlers);

  /**
   * \brief Copy all the linear constraints of the problem into the new problem.
   *
   * \param [in] p Input problem
   * \param [in] newp The transformed problem to which new constraints are
   * added.
   */
  void copyLinear_(ConstProblemPtr p, ProblemPtr newp);

  /**
   * \brief Copy all the linear constraints of the problem into the new problem.
   *
   * \param [in] p Input problem
   * \param [in] newp The transformed problem to which new constraints are
   * added.
   */
  void copyVars_(ConstProblemPtr p, ProblemPtr newp);

  /// Convert a maximization objective into minimization.
  void minObj_();

  /**
   * \brief Find the auxiliary variable associated with \f$y_i = x_j+d\f$ or
   * create a new one.
   *
   * \param [in] iv The variable \f$x_j\f$.
   * \param [in]  d The value \f$d\f$.
   * \param [in] newp The transformed problem to which the new constraint
   * should be added, in case this constraint is not found.
   * \return The variable \f$y\f$. If the constraint is found, it returns the
   * variable found in it, otherwise it addes a new variable to the problem
   * and returns it.
   */
  VariablePtr newVar_(VariablePtr iv, Double d, ProblemPtr newp);

  /**
   * \brief Find the auxiliary variable associated with \f$y_i = c^Tx+d\f$ or
   * create a new one.
   *
   * \param [in/out] lf The linear function \f$c^Tx\f$. If a new variable is
   * created, it is also subtracted from lf.
   * \param [in]  d The value \f$d\f$.
   * \param [in] newp The transformed problem to which the new constraint
   * should be added, in case this constraint is not found.
   * \return The variable \f$y\f$. If the constraint is found, it returns the
   * variable found in it, otherwise it addes a new variable to the problem
   * and returns it.
   */
  VariablePtr newVar_(LinearFunctionPtr lf, Double d, ProblemPtr newp);

  /**
   * \brief Find the auxiliary variable associated with \f$y_i = f(x)+d\f$ or
   * create a new one.
   *
   * \param [in] The nonlinear function. 
   * \param [in] newp The transformed problem to which the new constraint
   * should be added, in case this constraint is not found.
   * \return The variable \f$y\f$. If the constraint is found, it returns the
   * variable found in it, otherwise it addes a new variable to the problem
   * and returns it.
   */
  VariablePtr newVar_(CGraphPtr cg, ProblemPtr newp);

private:
  static const std::string me_;
    
};

typedef boost::shared_ptr<Transformer> TransformerPtr;
typedef boost::shared_ptr<const Transformer> ConstTransformerPtr;

}

#endif


// Local Variables: 
// mode: c++ 
// eval: (c-set-style "k&r") 
// eval: (c-set-offset 'innamespace 0) 
// eval: (setq c-basic-offset 2) 
// eval: (setq fill-column 78) 
// eval: (auto-fill-mode 1) 
// eval: (setq column-number-mode 1) 
// eval: (setq indent-tabs-mode nil) 
// End:
