/****************************************************************************
  FileName     [ cirFraig.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir FRAIG functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2012-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "sat.h"
#include "myHashMap.h"
#include "util.h"
#include <math.h>
#include <algorithm>

using namespace std;

// TODO: Please keep "CirMgr::strash()" and "CirMgr::fraig()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/
class WrapperGate{
 public:
  WrapperGate() {}
  WrapperGate( CirGate* g ) { pGate = g; sGate = reinterpret_cast< size_t >(g);}
  void merge( WrapperGate& gate ) {
    cout << "Strashing: " << pGate->getId() << " merging " 
         << gate.pGate->getId() << "..." << endl;
    cirMgr->replaceGate( gate.pGate, pGate );
  }
 private:
  size_t sGate;
  CirGate* pGate;
};
/**************************************/
/*   Static varaibles and functions   */
/**************************************/
static size_t HashKeyDigit = 13;
static size_t resim; 

template < typename T >
static void SWAP( T& a, T& b ) { T temp = a; a = b; b = temp; }

static size_t 
setReSimNum( size_t n ) {
  if ( n < 100 ) 
    return n;
  else 
    return sqrt( n ) / 10;
}
/*******************************************/
/*   Public member functions about fraig   */
/*******************************************/
// _floatList may be changed.
// _unusedList and _undefList won't be changed
void
CirMgr::strash()
{
  HashMap< HashKey, WrapperGate > hash( getHashSize( _netList.size() ) );
  for ( size_t i = 0 , n = _netList.size() ; i < n ; ++i ) {
    if ( _netList[ i ]->gateType() == AIG_GATE ) {
      HashKey k( toHashKey( _netList[ i ] ) );
      WrapperGate gate( _netList[ i ] ), mergeGate;
      if ( hash.check( k, mergeGate ) == true )  
        mergeGate.merge( gate );
      else hash.forceInsert( k, gate );
    }
  }
  dfsTraversal();
  assignFecGrp();
}

void
CirMgr::fraig()
{
  SatSolver solver; solver.initialize();
  size_t count = 0;
  genProofModel( solver );

  sortFecGrpList();
  while ( _fecGrpList.size() != 0 ) {
    for ( int i = _fecGrpList.size() - 1 ; i >= 0 ; --i ) { 
      if ( proofFecGrp( solver, _fecGrpList[ i ], count ) ) 
        _fecGrpList.pop_back();
      if ( ( count >= resim ) && ( _fecGrpList.size() > 0 ) ) {
        resimulate( count );
        count = 0; break;
      }
    }
  }

  dfsTraversal(); assignFecGrp();
}

/********************************************/
/*   Private member functions about fraig   */
/********************************************/
void
CirMgr::resimulate( size_t& count ) {
  for ( size_t j = 0, jn = ( count / 64 + 1 ) ; j < jn ; ++j ) {
    for ( size_t k = 0, kn = _piList.size() ; k < kn ; ++k )   
      _piList[ k ]->setSim( _satPattern[ j ][ k ] );
    dfsTraversal(); simulate(); identifyFEC(); 
  }
}


bool
CirMgr::proofFecGrp( SatSolver& solver, vector< CirGate* >& fecGrp, 
                     size_t& count ) { 
  int quit = 0, quitLimit = fecGrp.size() / 2;
  vector< bool > set;
  for ( size_t i = 0, in = fecGrp.size() ; i < in ; ++i ) 
    set.push_back( true );
  for ( size_t i = 0, in = fecGrp.size() ; i < in ; ++i ) {
    if ( !set[ i ] ) continue;
    for ( size_t j = i + 1, jn = fecGrp.size() ; j < jn ; ++j ) {
      if ( !set[ j ] ) continue;
      bool b;
      if ( fecGrp[ i ]->getSim() == fecGrp[ j ]->getSim() ) b = false;
      else                                                  b = true;

      if ( proofFecPair( solver, fecGrp[ i ], fecGrp[ j ], b ) ) {
        if ( fecGrp[ i ]->gateType() == AIG_GATE ) {
          if ( ( fecGrp[ i ]->_faninList[ 0 ] == fecGrp[ j ] ) ||
               ( fecGrp[ i ]->_faninList[ 1 ] == fecGrp[ j ] ) )
            SWAP( fecGrp[ i ], fecGrp[ j ] );
        }
        fraigMerge( fecGrp[ i ], fecGrp[ j ], b ); set[ j ] = false; 
      }
      else {
        setSatPattern( solver, count );
        ++quit;
        if ( quit == quitLimit ) return true; 
      }
    }
    //arrangeFecGrp( fecGrp, set );
  }
  return true;
}

void 
CirMgr::setSatPattern( SatSolver& solver, size_t& count ) {
  int index = count / 64;
  if ( index == _satPattern.size() ) 
    _satPattern.push_back( _satPattern[ 0 ] );
  for ( int k = 0, kn = _piList.size() ; k < kn ; ++k ) {
    _satPattern[ index ][ k ] = _satPattern[ index ][ k ] << 1;
    _satPattern[ index ][ k ] += solver.getValue( _piList[ k ]->getVar() );
  }
  ++count;
}

void 
CirMgr::arrangeFecGrp( vector< CirGate* >& fecGrp, vector< bool >& set ) {
  vector< CirGate* > temp;
  for ( size_t i = 0, in = fecGrp.size() ; i < in ; ++i )
    if ( set[ i ] == true ) 
      temp.push_back( fecGrp[ i ] );
  fecGrp.clear(); 
  fecGrp = temp;
}

void
CirMgr::fraigMerge( CirGate* newGate, CirGate* tempGate, bool ifec ) {
  cout << "Fraig: " << newGate->getId() << " merging ";
  if ( ifec ) cout << "!";
  cout << tempGate->getId() << "..." << endl;
  replaceGate( tempGate, newGate, ifec );
}

void
CirMgr::initSatPattern( size_t n ) {
  _satPattern.clear();
  vector< unsigned long long > temp;
  for ( int i = 0, in = _piList.size() ; i < in ; ++i )
    temp.push_back( 0 );
  for ( int i = 0, in = 100 ; i < in ; ++i ) 
    _satPattern.push_back( temp );
}

void 
CirMgr::resetSatPattern() {
  for ( size_t i = 0, in = _satPattern.size() ; i < in ; ++i ) 
    for ( size_t j = 0, jn = _piList.size() ; j < jn ; ++j )
      _satPattern[ i ][ j ] = 0;
}

vector< unsigned long long > 
CirMgr::toHashKey( CirGate* g ) {
  vector< CirGate* > f = g->_faninList;
  unsigned long long in0 = reinterpret_cast< unsigned long long >( f[ 0 ] ),
         in1 = reinterpret_cast< unsigned long long >( f[ 1 ] );
  bool b0 = g->invert( 0 ), b1 = g->invert( 1 );
  if ( in0 > in1 ) { SWAP( in0, in1); SWAP( b0, b1); }
  // digit = 2^( HashKeyDigit ) - 1, addInvert = 2^( HashKeyDigit )
  unsigned long long digit = 1, addInvert = 1;
  for ( int i = 0 ; i < ( HashKeyDigit + 1 ) ; ++i ) digit = digit << 1;
  digit -= 1;
  for ( int i = 0 ; i < HashKeyDigit ; ++i ) addInvert = addInvert << 1;
  in0 = in0 & digit;
  in1 = in1 & digit;
  if ( b0 ) in0 += addInvert;
  if ( b1 ) in1 += addInvert;
  vector< unsigned long long > s;
  s.push_back( in0 ); s.push_back( in1 );
  return s;
}

bool 
CirMgr::proofFecPair( SatSolver& solver, CirGate* g1, CirGate* g2, bool ifec ) {
  Var newV = solver.newVar();
  solver.addXorCNF(newV, g1->getVar(), false, g2->getVar(), ifec );
  solver.assumeRelease();  // Clear assumptions
  solver.assumeProperty( _totalList[ 0 ]->getVar(), false );
  solver.assumeProperty(newV, true);  // k = 1
  return !solver.assumpSolve();
}

void 
CirMgr::genProofModel( SatSolver& s ) {
  Var v = s.newVar();
  _totalList[ 0 ]->setVar( v );
  for (size_t i = 0, n = _netList.size(); i < n; ++i)
    if ( _netList[ i ]->gateType() != PO_GATE ) {
      Var v = s.newVar();    
      _netList[ i ]->setVar( v );
      if ( _netList[ i ]->gateType() == AIG_GATE ) {
        CirGate* g = _netList[ i ];
        s.addAigCNF( g->getVar(), 
                     g->_faninList[ 0 ]->getVar(), g->invert( 0 ), 
                     g->_faninList[ 1 ]->getVar(), g->invert( 1 ) );
      }
    }
  resim = setReSimNum( _netList.size() );
  initSatPattern( resim );
}

class WFec {
 public:
  WFec() {}
  WFec( unsigned long long a, vector< CirGate* > b ): d( a ), fecGrp( b ) {}
  unsigned long long d;
  vector< CirGate* > fecGrp;
};
bool 
wFecCompare( WFec a, WFec b ) { return ( a.d > b.d ); }
void 
CirMgr::sortFecGrpList() {
  for ( size_t i = 0, in = _netList.size() ; i < in ; ++i ) {
    if ( _netList[ i ]->gateType() == AIG_GATE ) {
      _netList[ i ]->_d = ( _netList[ i ]->_faninList[ 0 ]->_d > _netList[ i ]->_faninList[ 1 ]->_d )
                          ? ( _netList[ i ]->_faninList[ 0 ]->_d + 1 )
                          : ( _netList[ i ]->_faninList[ 1 ]->_d + 1 );
    }
    else if ( _netList[ i ]->gateType() == PI_GATE )
      _netList[ i ]->_d = 0;
    else if ( _netList[ i ]->gateType() == CONST_GATE )
      _netList[ i ]->_d = 0;
  }
  vector< WFec > temp;
  for ( size_t i = 0, in = _fecGrpList.size() ; i < in ; ++i ) {
    unsigned long long d = 0;
    for ( size_t j = 0, jn = _fecGrpList[ i ].size() ; j < jn ; ++j )
      d += _fecGrpList[ i ][ j ]->_d;
    temp.push_back( WFec( d, _fecGrpList[ i ] ) );
  }
  sort( temp.begin(), temp.end(), wFecCompare );
  for ( size_t i = 0, in = _fecGrpList.size() ; i < in ; ++i ) 
    _fecGrpList[ i ] = temp[ i ].fecGrp;
}
