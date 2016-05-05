/****************************************************************************
  FileName     [ cirGate.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define class CirAigGate member functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdarg.h>
#include <cassert>
#include "cirGate.h"
#include "cirMgr.h"
#include "util.h"

using namespace std;

// TODO: Keep "CirGate::reportGate()", "CirGate::reportFanin()" and
//       "CirGate::reportFanout()" for cir cmds. Feel free to define
//       your own variables and functions.

extern CirMgr *cirMgr;

/**************************************/
/*   class CirGate member functions   */
/**************************************/
unsigned CirGate::_globalRef = 0;
unsigned CirGate::_globalFraig = 0;
size_t CirGate::_mask = 0;

string int2str(int &i) {
  string s;
  stringstream ss(s);
  ss << i;
  return ss.str();
}

void
CirGate::reportGate() const
{
  stringstream ss;
  ss << _gateId;
  string id = ss.str();
  ss.str( "" ); ss.clear();
  ss << _lineNo;
  string no = ss.str();
  string output = "= " + getTypeStr() + "(" + id + ")";
  if ( _symbol != "" ) output += "\"" + _symbol + "\"";
  output += ", line " + no;
  cout << "==================================================" << endl;
  cout << setw(49) << left << output << "=" << endl;
  ss.str( "" ); ss.clear();
  output = "= FECs: ";
  for ( size_t i = 0, in = _fecGrp.size() ; i < in ; ++i ) { 
    if ( _fecGrp[ i ] != this ) {
      if ( _simValue != _fecGrp[ i ]->getSim() ) 
        output += "!";
      ss << _fecGrp[ i ]->getId();
      output += ss.str() + " ";
      ss.str( "" ); ss.clear();
    }
  }
  cout << setw(49) << left << output << "=" << endl;
  cout << "= Value: "; 
  size_t s = 0x80000000;
  for ( int i = 0 ; i < 32 ; ++i ) {
    if ( ( ( i % 4 ) == 0 ) && ( i != 0 ) )
      cout << "_";
    if ( s & _simValue ) cout << "1";
    else                 cout << "0";
    s = s >> 1;
  }
  cout << " =" << endl;
  cout << "==================================================" << endl;
}

void
CirGate::reportFanin(int level) const
{
   assert (level >= 0);
   CirGate::setGlobalRef();
   cout << getTypeStr() << " " << _gateId << endl; 
   if ( level > 0 ) {
     for ( int i = 0 ; i < _faninList.size() ; i++ ) {
       cout << "  "; 
       _faninList[ i ]->reportFanin( level - 1, 2, this->invert( i ) );
     }
   }
   this->setToGlobalRef();
}

void 
CirGate::reportFanin( int level, int time, bool invert ) const 
{
  assert( level >= 0 );
  if ( invert ) cout << "!";
  cout << getTypeStr() << " " << _gateId;
  if ( ( level != 0 ) && ( this->isGlobalRef() ) && ( _faninList.size() != 0 ) )
    cout << " (*)" << endl;
  else {
    cout << endl;
    if ( level > 0 ) {
      for ( int i = 0 ; i < _faninList.size() ; i++ ) {
        for ( int j = 0 ; j < time ; j++ ) cout << "  ";
        _faninList[ i ]->reportFanin( level - 1, time + 1, this->invert( i ) );
      }
      this->setToGlobalRef();
    }
  }
}

void
CirGate::reportFanout(int level) const
{
   assert (level >= 0);
   CirGate::setGlobalRef();
   cout << getTypeStr() << " " << _gateId << endl; 
   if ( level > 0 ) {
     for ( int i = 0 ; i < _fanoutList.size() ; i++ ) {
       cout << "  "; 
       _fanoutList[ i ]->reportFanout( level - 1, 2, this );
     }
   }
   this->setToGlobalRef();
}

void 
CirGate::reportFanout( int level, int time, const CirGate* parent ) const 
{
  assert( level >= 0 );
  int index;
  for ( int i = 0 ; i < _faninList.size() ; i++ ) 
    if ( _faninList[ i ] == parent ) 
      index = i;
  if ( this->invert( index ) ) cout << "!";
  cout << getTypeStr() << " " << _gateId;
  if ( ( level != 0 ) && ( this->isGlobalRef() ) && ( _fanoutList.size() != 0 ) )
    cout << " (*)" << endl;
  else {
    cout << endl;
    if ( level > 0 ) {
      for ( int i = 0 ; i < _fanoutList.size() ; i++ ) {
        for ( int j = 0 ; j < time ; j++ ) cout << "  ";
        _fanoutList[ i ]->reportFanout( level - 1, time + 1, this );
      }
      this->setToGlobalRef();
    }
  }
}

size_t
CirGate::getId() const 
{
  return _gateId;
}

GateType
CirGate::faninType( int index ) const
{
  return _faninList[ index ]->gateType();
}

int
CirGate::faninSize() const 
{
  return _faninList.size();
}

int
CirGate::fanoutSize() const 
{
  return _fanoutList.size();
}

void 
CirUndefGate::printGate() const {
  cout << "UNDEF" << endl;
}

void 
CirPiGate::printGate() const {
  cout << "PI  " << _gateId;
  if ( _symbol != "" ) 
    cout << " (" << _symbol << ")";
  cout << endl;
}

void 
CirPoGate::printGate() const {
  cout << "PO  " << _gateId;
  cout << " ";
  if ( _faninList[ 0 ]->gateType() == UNDEF_GATE ) 
    cout << "*";
  if ( _invert ) cout << "!";
  cout << _faninList[ 0 ]->getId();
  if ( _symbol != "" ) 
    cout << " (" << _symbol << ")";
  cout << endl;
}

void 
CirAigGate::printGate() const {
  cout << "AIG " << _gateId;
  for ( int i = 0 ; i < _faninList.size() ; i++ ) {
    cout << " ";
    if ( _faninList[ i ]->gateType() == UNDEF_GATE ) 
      cout << "*";
    if ( _invert[ i ] ) cout << "!";
    cout << _faninList[ i ]->getId();
  }
  cout << endl;
}

void 
CirConstGate::printGate() const {
  cout << "CONST0" << endl;
}

void 
CirUndefGate::simulate() {
  //return false;
}

void 
CirPiGate::simulate() {
  //return true;
}

void
CirPoGate::simulate() {
  _simValue = _faninList[ 0 ]->getSim();
  _simValue = ( _invert ) ? ( ~_simValue ) : ( _simValue );
}

void
CirAigGate::simulate() {
  unsigned long long in0 = _faninList[ 0 ]->getSim(), in1 = _faninList[ 1 ]->getSim();
  in0 = ( _invert[ 0 ] ) ? ( ~in0 ) : in0; 
  in1 = ( _invert[ 1 ] ) ? ( ~in1 ) : in1;
  _simValue = in0 & in1;
}

void
CirConstGate::simulate() {
}
