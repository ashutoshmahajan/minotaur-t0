// 
//     MINOTAUR -- It's only 1/2 bull
// 
//     (C)opyright 2008 - 2017 The MINOTAUR Team.
// 

/**
 * \file CplexLPEngine.h 
 * \brief Define an interface to the Cplex LP solver.
 * \author Prashant Palkar and Ashutosh Mahajan, IIT Bombay
 */

#ifndef MINOTAURCPLEXLPENGINE_H
#define MINOTAURCPLEXLPENGINE_H
//#include <ilcplex/ilocplex.h>
#include <ilcplex/cplexx.h>
#include <string.h>
#include "LPEngine.h"
#include "STOAHandler.h"


namespace Minotaur {

  class   Timer;
  class   Environment;
  class   Problem;
  class   Solution;
  class   WarmStart;
  typedef Environment* EnvPtr;
  typedef Problem* ProblemPtr;
  typedef Solution* SolutionPtr;
  typedef WarmStart* WarmStartPtr;

  /// Statistics
  struct CplexLPStats {
    UInt calls;     /// Total number of calls to solve.
    double time;    /// Sum of time taken in all calls to solve.
  };

  /// The CplexLPEngine class can be called to solve LP problems
  class CplexLPEngine : public LPEngine {
  public:
    
    //static int lazycallback(CPXCENVptr env, void *cbdata, int wherefrom,
             //void *cbhandle, int *useraction_p);
    
    void doNothing();
    
    /// Constructor with an environment.
    CplexLPEngine(EnvPtr env);

    /// Destroy. 
    ~CplexLPEngine();

    // Implement Engine::addConstraint().
    void addConstraint(ConstraintPtr);

    // Change bounds on a constraint.
    void changeBound(ConstraintPtr cons, BoundType lu, double new_val);

    // Implement Engine::changeBound(VariablePtr, BoundType, double).
    void changeBound(VariablePtr var, BoundType lu, double new_val);

    // Implement Engine::changeBound(VariablePtr, double, double).
    void changeBound(VariablePtr var, double new_lb, double new_ub);

    // Implement Engine::changeConstraint().
    void changeConstraint(ConstraintPtr con, LinearFunctionPtr lf, 
                          double lb, double ub);

    // Implement Engine::changeConstraint().
    void changeConstraint(ConstraintPtr c, NonlinearFunctionPtr nlf);

    // change the objective function.
    void changeObj(FunctionPtr f, double cb);

    /// Clear the problem.
    void clear();

    void disableStrBrSetup() {};

    /// Return an empty CplexLPEngine pointer.
    EnginePtr emptyCopy();

    void enableStrBrSetup() {};

    /// Return the solution value of the objective after solving the LP.
    double getSolutionValue();

    // Implement Engine::getSolution().
    ConstSolutionPtr getSolution();

    // Implement Engine::getStatus().
    EngineStatus getStatus();

    // get name.
    std::string getName() const;

    ConstWarmStartPtr getWarmStart() {return WarmStartPtr();};

    WarmStartPtr getWarmStartCopy()  {return WarmStartPtr();};

    /** 
     * Load the problem into the engine. We create arrays of variables and
     * constraints, the A matrix, rhs, objective etc from the problem and
     * initialize the LP solver.
     */
    void load(ProblemPtr problem);

    void loadFromWarmStart(const WarmStartPtr ) {};

    // Convert 'min f' to 'min -f'.
    void negateObj();

    // Print a point x
    void printx(double *, UInt );
    
    // base class method.
    void removeCons(std::vector<ConstraintPtr> &delcons);

    // Implement Engine::resetIterationLimit().
    void resetIterationLimit();

    // Implement Engine::setIterationLimit().
    void setIterationLimit(int);

    // Implement Engine::setTimeLimit().
    void setTimeLimit(double);

    // Implement the LP solve() function of Cplex
    EngineStatus solve();

    /// Writes an LP file of the loaded LP.
    void writeLP(const char *filename) const;

    /// Write statistics.
    void writeStats(std::ostream &out) const;

  private:

    /// Cplex Environment
    CPXENVptr cpxenv_;

    /// Cplex Problem pointer
    CPXLPptr cpxlp_;
   
    /// Cplex Status
    int cpxstatus_;

    /// Time limit for the iteration
    double timeLimit_;

    /// True if any bound on variable on constraint was changed after 
    /// previous solve.
    bool bndChanged_;

    /// True if a constraint (not it bound) was changed after previous solve.
    bool consChanged_;

    /// Environment.
    EnvPtr env_;

    /// The maximum limit that can be set on the solver. 
    int maxIterLimit_;

    /// String name used in log messages.
    static const std::string me_;

    /// True if objective was changed after previous solve.
    bool objChanged_;

    /// Problem that is loaded, if any.
    ProblemPtr problem_;

    /// Solution.
    SolutionPtr sol_;

    /// Statistics.
    CplexLPStats *stats_;

    /// Timer for solves. 
    Timer *timer_;

    /// Actually load the problem. Called before _every_ solve.
    void load_();
  };
  
  typedef CplexLPEngine* CplexLPEnginePtr;
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