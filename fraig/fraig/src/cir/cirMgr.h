/****************************************************************************
  FileName     [ cirMgr.h ]
  PackageName  [ cir ]
  Synopsis     [ Define circuit manager ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_MGR_H
#define CIR_MGR_H

#include <vector>
#include <string>
#include <fstream>
#include <iostream>

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

#include "cirDef.h"
#include "sat.h"

extern CirMgr *cirMgr;

class CirMgr
{
  friend class WrapperGate;
public:
   CirMgr() {}
   ~CirMgr() { clear(); } 

   // Access functions
   // return '0' if "gid" corresponds to an undefined gate.
   CirGate* getGate(unsigned gid) const { return _totalList[ gid ]; }

   // Member functions about circuit construction
   bool readCircuit(const string&);

   // Member functions about circuit optimization
   void sweep();
   void optimize();

   // Member functions about simulation
   void randomSim();
   void fileSim(ifstream&);
   void setSimLog(ofstream *logFile) { _simLog = logFile; }

   // Member functions about fraig
   void strash();
   void printFEC() const;
   void fraig();

   // Member functions about circuit reporting
   void printSummary() const;
   void printNetlist() const;
   void printPIs() const;
   void printPOs() const;
   void printFloatGates() const;
   void printFECPairs() const;
   void writeAag(ostream&) const;
   void writeGate(ostream&, CirGate*) const;

private:
   ofstream           *_simLog;

   GateList _piList;
   GateList _poList;
   GateList _aigList;//not maintain in strash
   GateList _netList;
   GateList _totalList;
   GateList _fraigList;

   vector< vector< unsigned long long > > _satPattern;

   int _maxVariables;
   vector< string > _symbolInput;

   vector< GateList > _fecGrpList;

   vector< int > MILOA( const string& );
   void createFanoutList( vector< vector< unsigned > >, int );
   void parseSymbol( ifstream& );

   void dfsTraversal();
   void dfsTraversal( CirGate* );
   void createConstGate();
   void setFanoutList( unsigned, unsigned );
   void clear();
   //sweep, optimize
   void clearUnusedFanout( CirGate* );
   void aigOptConstFanin( CirGate*, int );
   void aigOptSameFanin( CirGate* ); 
   void inverseFanoutInvert( CirGate* );
   void replaceGate( CirGate*, CirGate*, bool = false );

   //strash
   //vector< unsigned long long > toHashKey( CirGate* );

   //simulate
   void determineMaxFail();
   void createRandomSim();
   void simulate();
   void initFecGrp();
   void identifyFEC();
   void outputFile( vector< string >& , size_t& );
   bool checkErr( vector< string > );
   void stringToSim( vector< string >& , size_t& );
   void sortFecGrp();
   void assignFecGrp();

   void genProofModel( SatSolver& );
   bool proofFecGrp( SatSolver&, vector< CirGate* >&, size_t& );
   void arrangeFecGrp( vector< CirGate* >&, vector< bool >& );
   bool proofFecPair( SatSolver&, CirGate*, CirGate*, bool = false );
   bool proofConstPair( SatSolver&, CirGate*, bool = false );
   void fraigMerge( CirGate*, CirGate*, bool = false );

   void initSatPattern( size_t );
   void setSatPattern( SatSolver&, size_t& );
   void resetSatPattern();
   void resimulate( size_t& );

   void writedfs( CirGate*, vector< CirGate* >&, vector< CirGate* >& ) const;
   //speed up
   void sortFecGrpList();
   bool proofSATGrp( SatSolver&, vector< CirGate* >& );
   void mergeGrp( vector< CirGate* >& );
};

#endif // CIR_MGR_H
