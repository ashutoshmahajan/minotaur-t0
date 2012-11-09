//
//    MINOTAUR -- It's only 1/2 bull
//
//    (C)opyright 2009 -- 2010 The MINOTAUR Team.
//

/**
 * \file Bnb.cpp
 * \brief The main function for solving instances in ampl format (.nl) by
 * using Branch-and-Bound alone.
 * \author Ashutosh Mahajan, Argonne National Laboratory
 */

#include <iomanip>
#include <iostream>

#include "MinotaurConfig.h"
#include "BranchAndBound.h"
#include "Engine.h"
#include "EngineFactory.h"
#include "Environment.h"
#include "LexicoBrancher.h"
#include "LinearHandler.h"
#include "LPEngine.h"
#include "LPProcessor.h"
#include "MaxVioBrancher.h"
#include "NodeIncRelaxer.h"
#include "NLPEngine.h"
#include "NlPresHandler.h"
#include "Objective.h"
#include "Option.h"
#include "Presolver.h"
#include "ProblemSize.h"
#include "Problem.h"
#include "Relaxation.h"
#include "ReliabilityBrancher.h"
#include "SimpleTransformer.h"
#include "Solution.h"
#include "Timer.h"
#include "Transformer.h"
#include "TreeManager.h"

#include "AMPLInterface.h"

using namespace Minotaur;

BranchAndBound *createBab (EnvPtr env, ProblemPtr p, EnginePtr e, 
                           HandlerVector &handlers);
PresolverPtr createPres(EnvPtr env, ProblemPtr p, Size_t ndefs, 
                        HandlerVector &handlers);
EnginePtr getEngine(EnvPtr env);
void show_help();



void show_help()
{
  std::cout << "Usage:" << std::endl
            << "To show version: glob -v (or --show_version yes) " << std::endl
            << "To show all options: glob -= (or --show_options yes)" 
            << std::endl
            << "To solve an instance: glob --option1 [value] "
            << "--option2 [value] ... " << " .nl-file" << std::endl;
}


int main(int argc, char** argv)
{
  EnvPtr env      = (EnvPtr) new Environment();
  Timer *timer    = env->getNewTimer();
  OptionDBPtr options;
  MINOTAUR_AMPL::AMPLInterfacePtr iface;
  ProblemPtr inst;     // instance that needs to be solved.
  EnginePtr engine;    // engine for solving relaxations. 
  SolutionPtr sol, sol2;
  BranchAndBound *bab = 0;
  PresolverPtr pres;
  const std::string me("glob: ");
  VarVector *orig_v=0;
  HandlerVector handlers;
  Double obj_sense = 1.0;

  // start timing.
  timer->start();

  iface = (MINOTAUR_AMPL::AMPLInterfacePtr) 
    new MINOTAUR_AMPL::AMPLInterface(env, "glob");

  // parse options
  options = env->getOptions();
  options->findString("interface_type")->setValue("AMPL");
  options->findBool("presolve")->setValue(true);
  options->findBool("nl_presolve")->setValue(true);
  options->findBool("lin_presolve")->setValue(true);
  env->readOptions(argc, argv);

  // check if user needs help.
  if (options->findBool("show_options")->getValue() ||
      options->findFlag("=")->getValue()) {
    options->write(std::cout);
    goto CLEANUP;
  }

  if (options->findBool("show_help")->getValue() ||
      options->findFlag("?")->getValue()) {
    show_help();
    goto CLEANUP;
  }

  if (options->findBool("show_version")->getValue() ||
      options->findFlag("v")->getValue()) {
    std::cout << me << "Minotaur version " << env->getVersion() << std::endl;
#if DEBUG
    std::cout << me; 
    env->writeFullVersion(std::cout);
    std::cout << std::endl;
#endif
    goto CLEANUP;
  }

  if (options->findString("problem_file")->getValue()=="") {
    show_help();
    goto CLEANUP;
  }

  std::cout << me << "Minotaur version " << env->getVersion() << std::endl;
  if (options->findBool("use_native_cgraph")->getValue()==false) {
    options->findBool("use_native_cgraph")->setValue(true); 
    std::cout << me << "Setting value of 'use_native_cgraph option' to True" <<
      std::endl;
  }

  // load the problem.
  inst = iface->readInstance(options->findString("problem_file")->getValue());
  std::cout << "time used in reading instance = " << std::fixed 
    << std::setprecision(2) << timer->query() << std::endl;

  // display the problem
  inst->calculateSize();
  if (options->findBool("display_problem")->getValue()==true) {
    inst->write(std::cout, 9);
  }
  if (options->findBool("display_size")->getValue()==true) {
    inst->writeSize(std::cout);
  }

  // Get the right engine.
  engine = getEngine(env);
  std::cout << me << "engine used = " << engine->getName() << std::endl;

  if (inst->getObjective() &&
      inst->getObjective()->getObjectiveType()==Maximize) {
    obj_sense = -1.0;
  }

  // get presolver.
  handlers.clear();
  orig_v = new VarVector(inst->varsBegin(), inst->varsEnd());
  pres = createPres(env, inst, iface->getNumDefs(), handlers);
  if (env->getOptions()->findBool("presolve")->getValue() == true) {
    std::cout << me << "Presolving ... " << std::endl;
    pres->solve();
    std::cout << me << "Finished presolving." << std::endl;
    for (HandlerVector::iterator it=handlers.begin(); it!=handlers.end(); ++it) {
      (*it)->writeStats(std::cout);
    }
  }
  handlers.clear();

  // get branch-and-bound
  bab = createBab(env, inst, engine, handlers);

  // solve
  if (options->findBool("solve")->getValue()==true) {
    // start solving
    bab->solve();

    std::cout << me << "status of branch-and-bound: " 
      << getSolveStatusString(bab->getStatus()) << std::endl;
    // write solution
    sol = bab->getSolution(); // presolved solution needs translation.
    if (sol) {
      sol2 = pres->getPostSol(sol);
      sol = sol2;
    }
    if (options->findFlag("AMPL")->getValue()) {
      iface->writeSolution(sol, bab->getStatus());
    } else if (sol) {
      sol->writePrimal(std::cout,orig_v);
    }
    std::cout << me << "nodes created in branch-and-bound = " << 
      bab->getTreeManager()->getSize() << std::endl;
    std::cout << me << std::fixed << std::setprecision(4) << 
      "best bound estimate from remaining nodes = "
      <<  obj_sense*bab->getLb() << std::endl;
    std::cout << me << std::fixed << std::setprecision(4) << 
      "best solution value = " << obj_sense*bab->getUb() << std::endl;
    // bit->writeStats();
    for (HandlerVector::iterator it=handlers.begin(); it!=handlers.end(); ++it) {
      (*it)->writeStats(std::cout);
    }
  }
  std::cout << me << "time used = " << std::fixed << std::setprecision(2) 
    << timer->query() << std::endl;

  engine->writeStats();
  bab->getNodeProcessor()->getBrancher()->writeStats();

CLEANUP:
  if (iface) {
    delete iface;
  }
  delete timer;
  if (bab) {
    delete bab;
  }
  if (orig_v) {
    delete orig_v;
  }

  return 0;
}


EnginePtr getEngine(EnvPtr env)
{
  EngineFactory *efac = new EngineFactory(env);
  EnginePtr e = EnginePtr(); // NULL
  e = efac->getLPEngine();
  if (e) {
    delete efac;
    return e;
  } 

  std::cout << "Cannot find an LP engine. Cannot proceed!" << std::endl;
  return e;
}


BranchAndBound * createBab (EnvPtr env, ProblemPtr p, EnginePtr e, 
                            HandlerVector &handlers)
{
  BranchAndBound *bab = new BranchAndBound(env, p);
  LPProcessorPtr nproc;
  NodeIncRelaxerPtr nr;
  RelaxationPtr rel;
  BrancherPtr br;
  SimpTranPtr trans = SimpTranPtr();
  ProblemPtr newp;
  Int status = 0;
  const std::string me("glob: ");

  handlers.clear();
  trans = (SimpTranPtr) new SimpleTransformer(env);
  trans->reformulate(p, newp, handlers, status);
  assert(0==status);
  std::cout << me << "handlers used: " << std::endl;
  for (HandlerVector::iterator it=handlers.begin(); it!=handlers.end();
       ++it) {
    std::cout << "  " << (*it)->getName() << std::endl;
  }
  nproc = (LPProcessorPtr) new LPProcessor(env, e, handlers);
  if (env->getOptions()->findString("brancher")->getValue() == "rel") {
    UInt t;
    ReliabilityBrancherPtr rel_br;
    rel_br = (ReliabilityBrancherPtr) new ReliabilityBrancher(env, handlers);
    rel_br->setEngine(e);
    t = (p->getSize()->ints + p->getSize()->bins)/10;
    t = std::max(t, (UInt) 2);
    t = std::min(t, (UInt) 4);
    rel_br->setThresh(t);
    std::cout << me << "setting reliability threshhold to " << t << std::endl;
    t = (UInt) p->getSize()->ints + p->getSize()->bins/20+2;
    t = std::min(t, (UInt) 10);
    rel_br->setMaxDepth(t);
    std::cout << me << "setting reliability maxdepth to " << t << std::endl;
    std::cout << me << "reliability branching iteration limit = " 
              << rel_br->getIterLim() << std::endl;
    br = rel_br;
  } else if (env->getOptions()->findString("brancher")->getValue() 
      == "maxvio") {
    MaxVioBrancherPtr mbr = 
      (MaxVioBrancherPtr) new MaxVioBrancher(env, handlers);
    br = mbr;
  } else if (env->getOptions()->findString("brancher")->getValue() 
      == "lex") {
    LexicoBrancherPtr lbr = 
      (LexicoBrancherPtr) new LexicoBrancher(env, handlers);
    br = lbr;
  }
  std::cout << me << "brancher used = " << br->getName() << std::endl;
  nproc->setBrancher(br);
  bab->setNodeProcessor(nproc);

  nr = (NodeIncRelaxerPtr) new NodeIncRelaxer(env, handlers);
  nr->setRelaxation(rel);
  nr->setEngine(e);
  bab->setNodeRelaxer(nr);
  bab->shouldCreateRoot(true);

  // heuristic
  return bab;
}


PresolverPtr createPres(EnvPtr env, ProblemPtr p, Size_t ndefs, 
                        HandlerVector &handlers)
{
  // create handlers for presolve
  PresolverPtr pres = PresolverPtr(); // NULL
  const std::string me("glob: ");

  p->calculateSize();
  if (env->getOptions()->findBool("presolve")->getValue() == true) {
    LinearHandlerPtr lhandler = (LinearHandlerPtr) new LinearHandler(env, p);
    handlers.push_back(lhandler);
    if (p->isQP() || p->isQuadratic() || p->isLinear() ||
        true==env->getOptions()->findBool("use_native_cgraph")->getValue()) {
      lhandler->setPreOptPurgeVars(true);
      lhandler->setPreOptPurgeCons(true);
      lhandler->setPreOptCoeffImp(true);
    } else {
      lhandler->setPreOptPurgeVars(false);
      lhandler->setPreOptPurgeCons(false);
      lhandler->setPreOptCoeffImp(false);
    }
    if (ndefs>0) {
      lhandler->setPreOptDualFix(false);
    } else {
      lhandler->setPreOptDualFix(true);
    }

    if (!p->isLinear() && 
         true==env->getOptions()->findBool("use_native_cgraph")->getValue() && 
         true==env->getOptions()->findBool("nl_presolve")->getValue() 
         ) {
      NlPresHandlerPtr nlhand = (NlPresHandlerPtr) new NlPresHandler(env, p);
      handlers.push_back(nlhand);
    }

    // write the names.
    std::cout << me << "handlers used in presolve:" << std::endl;
    for (HandlerIterator h = handlers.begin(); h != handlers.end(); 
        ++h) {
      std::cout<<(*h)->getName()<<std::endl;
    }
  }

  pres = (PresolverPtr) new Presolver(p, env, handlers);
  return pres;
}

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
