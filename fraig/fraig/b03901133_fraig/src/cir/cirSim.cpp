/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir simulation functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"
#include <sstream>
#include <bitset>
#include <math.h>

using namespace std;

// TODO: Keep "CirMgr::randimSim()" and "CirMgr::fileSim()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/
static int MaxFail = 0;
static int SizeT = sizeof( unsigned long long ) * 8;

static bool
forSortFecGrp( CirGate* a, CirGate* b ) {
  return ( a->getId() < b->getId() );
}
/************************************************/
/*   Public member functions about Simulation   */
/************************************************/
void
CirMgr::randomSim()
{
  initFecGrp();
  int patternNum = 0;
  determineMaxFail();
  for ( int count = 0 ; count < MaxFail ; ++count, patternNum += 64 ) {
    createRandomSim();
    simulate();
    identifyFEC();
  }
  cout << patternNum << " patterns simulated." << endl;
  sortFecGrp();
  assignFecGrp();
}

void
CirMgr::fileSim(ifstream& patternFile)
{
  initFecGrp();
  vector< unsigned long long > simValue; bool err = false; 
  string temp; vector< string > input;
  //while ( getline( patternFile, temp, '\n' ) )
  while ( patternFile >> temp )
    input.push_back( temp );
  if ( !checkErr( input ) ) err = true;
  if ( !err ) {
    for ( size_t done = 0, inputSize = input.size(); done<inputSize; done+=SizeT)    {
      stringToSim( input, done );
      simulate(); 
      identifyFEC();
      if ( _simLog ) outputFile( input, done );
    }
    cout << input.size() << " patterns simulated." << endl;
    sortFecGrp();
    assignFecGrp();
  }
}

/*************************************************/
/*   Private member functions about Simulation   */
/*************************************************/
void 
CirMgr::determineMaxFail() {
  MaxFail = 10;
  if ( _netList.size() < 100 ) 
    MaxFail = _netList.size();
  else 
    MaxFail = sqrt( _netList.size() );
}

void 
CirMgr::createRandomSim() {
  RandomNumGen g( 0 );
  for ( size_t i = 0, in = _piList.size() ; i < in ; ++i ) {
    unsigned long long temp = g( INT_MAX );
    temp = temp << 32;
    temp += g( INT_MAX );
    _piList[ i ]->setSim( temp );
  }
}

void 
CirMgr::simulate() {
  //CirGate::setGlobalRef();
  //for ( size_t i = 0, on = _poList.size() ; i < on ; ++i )
  //  _poList[ i ]->simulate();
  for ( size_t i = 0, in = _netList.size() ; i < in ; ++i ) 
    _netList[ i ]->simulate();
}

void
CirMgr::initFecGrp() 
{
  if ( _fecGrpList.size() == 0 ) {
    vector< CirGate* > fecGrp;
    fecGrp.push_back( _totalList[ 0 ] );
    for ( int i = 0, n = _netList.size() ; i < n ; ++i )
      if ( _netList[ i ]->gateType() == AIG_GATE ) 
        fecGrp.push_back( _netList[ i ] );
    _fecGrpList.push_back( fecGrp );
  }
}

bool 
CirMgr::identifyFEC() 
{
  size_t temp = _fecGrpList.size();
  for ( size_t i = 0, in = _fecGrpList.size() ; i < in ; ++i ) {
    HashMap< HashKey, CirGate* > newFecGrps( getHashSize( _fecGrpList[ i ].size() ) );
    for ( size_t j = 0, jn = _fecGrpList[ i ].size() ; j < jn ; ++j ) {
      HashKey k( _fecGrpList[ i ][ j ]->getSim() );
      CirGate* grp;
      if ( newFecGrps.check( k, grp ) )    
        _fecGrpList[ grp->_grpNo ].push_back( _fecGrpList[ i ][ j ] );
      else {
        _fecGrpList[ i ][ j ]->_grpNo = _fecGrpList.size();
        vector< CirGate* > temp;
        temp.push_back( _fecGrpList[ i ][ j ] );
        _fecGrpList.push_back( temp );
        newFecGrps.forceInsert( k, _fecGrpList[ i ][ j ] );
      }
    }
  }
  vector< vector< CirGate* > > tempGrpList;
  for ( size_t i = temp, in = _fecGrpList.size() ; i < in ; ++i )
    if ( _fecGrpList[ i ].size() > 1 )  
      tempGrpList.push_back( _fecGrpList[ i ] );
  _fecGrpList.clear();
  _fecGrpList = tempGrpList;
  return !( _fecGrpList.size() == temp );
}

void
CirMgr::outputFile( vector< string >& input, size_t& done ) {
  unsigned long long pos = 1;
  unsigned long long in = ( ( input.size() - done ) > SizeT ) ? ( done + 64 ) : input.size();  
  for ( size_t i = done ; i < in ; ++i ) {
    *_simLog << input[ i ] << " ";
    for ( size_t j = 0, jn = _poList.size() ; j < jn ; ++j ) {
      if ( _poList[ j ]->getSim() & pos ) *_simLog << "1";
      else                                *_simLog << "0";
    }
    *_simLog << endl;
    pos = pos << 1;
  }
}

bool
CirMgr::checkErr( vector< string > input ) {
  for ( int i = 0, n = input.size(), in = _piList.size() ; i < n ; ++i )
    if ( input[ i ].size() != in ) {
      cout << "line " << i + 1 << " : ";
      cout << "Error: Pattern(" << input[ i ] 
           << ") length(" << input[ i ].size() 
           << ") does not match the number of inputs(" << in
           << ") in a circuit!!" << endl;
      return false;
    }
  for ( int  i = 0, in = input.size() ; i < in ; ++i )
    for ( int j = 0, jn = input[ i ].size() ; j < jn ; ++j )
      if ( input[ i ][ j ] != '0' && input[ i ][ j ] != '1' ) { 
        cout << "Error: Pattern(" << input[ i ] 
             << ") contains a non-0/1 character(\'"
             << input[ i ][ j ] << ")." << endl;
        return false;
      }
  return true;
}

//done should not be changed
void
CirMgr::stringToSim( vector< string >& input, size_t& done ) {
  size_t jn = ( ( input.size() - done ) > SizeT ) ?( done+SizeT ):input.size(); 
  string s; stringstream ss;
  for ( size_t i = 0, in = _piList.size() ; i < in ; ++i ) {
    s = "";
    for ( size_t j = ( jn - 1 ) ; j >= done ; --j ) {
      s += input[ j ][ i ];
      if ( ( j == 0 ) && ( done == 0 ) ) break;
    }
    bitset< sizeof( unsigned long long ) * 8 > sn( s );
    unsigned long long temp = sn.to_ulong();
    stringstream ss;
    ss << hex << temp;
    _piList[ i ]->setSim( temp );
    ss.str( "" ); ss.clear();
  }
}

void 
CirMgr::sortFecGrp() {
  for ( size_t i = 0, in = _fecGrpList.size() ; i < in ; ++i )  
    sort( _fecGrpList[ i ].begin(), _fecGrpList[ i ].end(), forSortFecGrp ); 
}

void 
CirMgr::assignFecGrp() {
  CirGate::setGlobalRef();
  for ( size_t i = 0, in = _fecGrpList.size() ; i < in ; ++i )   
    for ( size_t j = 0, jn = _fecGrpList[ i ].size() ; j < jn ; ++j ) {
      _fecGrpList[ i ][ j ]->setToGlobalRef();
      _fecGrpList[ i ][ j ]->_fecGrp = _fecGrpList[ i ];
    }
  for ( size_t i = 0, in = _totalList.size() ; i < in ; ++i )
    if ( _totalList[ i ] )
      if ( !_totalList[ i ]->isGlobalRef() )  
        _totalList[ i ]->_fecGrp.clear();
}
