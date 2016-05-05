/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir optimization functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::sweep()" and "CirMgr::optimize()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/**************************************************/
/*   Public member functions about optimization   */
/**************************************************/
// Remove unused gates
// DFS list should NOT be changed
// UNDEF, float and unused list may be changed
void
CirMgr::sweep()
{
  CirGate::setGlobalRef();
  _totalList[ 0 ]->setToGlobalRef();
  for ( int i = 0, n = _netList.size() ; i < n ; ++i ) 
    _netList[ i ]->setToGlobalRef();
  for ( int i = 0, in = _piList.size() ; i < in ; ++i )
    _piList[ i ]->setToGlobalRef();
  for ( int i = 0, tn = _totalList.size() ; i < tn ; ++i ) {
    if ( _totalList[ i ] ) {
      if ( !_totalList[ i ]->isGlobalRef() ) {
        _totalList[ i ]->_fanoutList.clear();
        for ( int j = 0 ; j < _totalList[ i ]->faninSize() ; ++j ) {
          if ( _totalList[ i ]->_faninList[ j ]->isGlobalRef() )
            clearUnusedFanout( _totalList[ i ]->_faninList[ j ] );
        }
        _totalList[ i ]->_faninList.clear();
        cout << "Sweeping: " << _totalList[ i ]->getTypeStr() 
             << "(" << _totalList[ i ]->getId() << ") removed..." << endl;
        delete _totalList[ i ];
        _totalList[ i ] = NULL;
      }
    }
  }
}
// Recursively simplifying from POs;
// _dfsList needs to be reconstructed afterwards
// UNDEF gates may be delete if its fanout becomes empty...
void
CirMgr::optimize()
{
  for ( int i = 0, n = _netList.size() ; i < n ; ++i ) {
    if ( _netList[ i ]->gateType() == AIG_GATE ) {
      if ( _netList[ i ]->_faninList[ 0 ]->gateType() == CONST_GATE ) 
        aigOptConstFanin( _netList[ i ], 0 );
      else if ( _netList[ i ]->_faninList[ 1 ]->gateType() == CONST_GATE )
        aigOptConstFanin( _netList[ i ], 1 );
      else if ( _netList[ i ]->_faninList[0] == _netList[ i ]->_faninList[1] ) {
        aigOptSameFanin( _netList[ i ] );
      }
    }
  }
  dfsTraversal();
}

/***************************************************/
/*   Private member functions about optimization   */
/***************************************************/
void 
CirMgr::clearUnusedFanout( CirGate* g ) {
  int gFanoutSize = g->fanoutSize(), flag = 0;
  for ( int i = 0 ; i < gFanoutSize ; ++i ) {
    if ( !g->_fanoutList[ flag ]->isGlobalRef() ) 
      g->_fanoutList.erase( g->_fanoutList.begin() + flag );
    else ++flag;
  }
}

void 
CirMgr::aigOptConstFanin( CirGate* g, int index ) {
  //const 1
  if ( g->invert( index ) ) {
    CirGate* fanin;
    if ( index == 0 ) fanin = g->_faninList[ 1 ];
    else              fanin = g->_faninList[ 0 ];
    cout << "Simplifying: " << fanin->getId() << " merging ";
    if ( ( ( index == 0 ) && ( g->invert( 1 ) ) ) || 
         ( ( index == 1 ) && ( g->invert( 0 ) ) ) ) {
      inverseFanoutInvert( g );
      cout << "!";
    }
    cout << g->getId() << "..." << endl;
    replaceGate( g, fanin );
  }
  //const 0
  else { 
    cout << "Simplifying: 0 merging " << g->getId() << "..." << endl; 
    replaceGate( g, _totalList[ 0 ] );
  }
}

void 
CirMgr::aigOptSameFanin( CirGate* g ) {
  if ( g->invert( 0 ) == g->invert( 1 ) ) { 
    cout << "Simplifying: " << g->_faninList[ 0 ]->getId() << " merging ";  
    if ( g->invert( 0 ) ) {
      inverseFanoutInvert( g );
      cout << "!";
    }
    cout << g->getId() << "..." << endl;
    replaceGate( g, g->_faninList[ 0 ] );
  }
  else {
    cout << "Simplifying: 0 merging " << g->getId() << "..." << endl; 
    replaceGate( g, _totalList[ 0 ] );
  }
}

void 
CirMgr::inverseFanoutInvert( CirGate* g ) {
  for ( int i = 0, on = g->fanoutSize() ; i < on ; ++i )  
    for ( int j = 0, in = g->_fanoutList[ i ]->faninSize() ; j < in ; ++j )  
      if ( g->_fanoutList[ i ]->_faninList[ j ] == g )   
        g->_fanoutList[ i ]->inverseInvert( j );
}

void 
CirMgr::replaceGate( CirGate* tempGate, CirGate* newGate, bool ifec ) {
  for ( int i = 0, in = tempGate->faninSize() ; i < in ; ++i ) { 
    int size = tempGate->_faninList[ i ]->fanoutSize(), flag = 0;
    CirGate* g = tempGate->_faninList[ i];
    for ( int j = 0 ; j < size ; ++j ) {
      if ( g->_fanoutList[ flag ] == tempGate ) 
        g->_fanoutList.erase( g->_fanoutList.begin() + flag );
      else ++flag;
    }
  }
  for ( int i = 0, in = tempGate->fanoutSize() ; i < in ; ++i ) 
    for ( int j = 0, jn = tempGate->_fanoutList[ i ]->faninSize() ; j < jn ; ++j ) 
      if ( tempGate->_fanoutList[ i ]->_faninList[ j ] == tempGate ) { 
        tempGate->_fanoutList[ i ]->_faninList[ j ] = newGate;
        if ( ifec )
          tempGate->_fanoutList[ i ]->inverseInvert( j );
      }
  for ( int i = 0, on = tempGate->fanoutSize() ; i < on ; ++i )   
    newGate->_fanoutList.push_back( tempGate->_fanoutList[ i ] );
  //tempGate->_fanoutList.clear();
  //tempGate->_faninList.clear();
  delete _totalList[ tempGate->getId() ];
  _totalList[ tempGate->getId() ] = NULL;
}
