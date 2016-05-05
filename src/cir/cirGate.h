/****************************************************************************
  FileName     [ cirGate.h ]
  PackageName  [ cir ]
  Synopsis     [ Define basic gate data structures ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_GATE_H
#define CIR_GATE_H

#include <string>
#include <vector>
#include <iostream>
#include "cirDef.h"
#include "sat.h"

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

class CirGate;

//------------------------------------------------------------------------
//   Define classes
//------------------------------------------------------------------------
class CirGate
{
  friend class CirMgr;
public:
   CirGate(){}
   CirGate( size_t id, unsigned n ): 
   _ref( 0 ), _gateId( id ), _symbol( "" ), _lineNo( n ) {}
   CirGate( size_t id, GateList i, unsigned n ): 
   _ref( 0 ), _gateId( id ), _faninList( i ), _symbol( "" ), _lineNo( n ) {}
   virtual ~CirGate() {}

   // Basic access methods
   virtual string getTypeStr() const = 0;
   void setSymbolStr( string s ) { _symbol = s; }
   string getSymbol() { return _symbol; }
   unsigned getLineNo() const { return _lineNo; }
   virtual bool isAig() const { return false; }//TODO

   // Printing functions
   virtual void printGate() const = 0;
   void reportGate() const;
   void reportFanin(int level) const;
   void reportFanin(int level, int time, bool invert ) const;
   void reportFanout(int level) const;
   void reportFanout(int level, int time, const CirGate* parent ) const;

   virtual GateType gateType() const = 0;
   virtual size_t getLId() const = 0;
   virtual bool invert( int = 0 ) const = 0;
   size_t getId() const;
   GateType faninType( int index ) const;
   int  faninSize() const;
   int fanoutSize() const;

   bool isGlobalRef() const { return ( _ref == _globalRef ); }
   void setToGlobalRef() const { _ref = _globalRef; }
   static void setGlobalRef() { _globalRef++; }
   
   virtual void inverseInvert( int index ) { }
   virtual vector< bool > getInvertVector() { vector< bool > b; return b; }

   virtual void setSim( unsigned long long ) { } //for CirPiGate
   virtual unsigned long long getSim() { return _simValue; }
   virtual void simulate() = 0;

   static void setMaskNum( size_t s ) { _mask = s; }
   
   Var getVar() const { return _var; }
   void setVar(const Var& v) { _var = v; }

   size_t _d;

   bool isFraig() const { return ( _fraig == _globalFraig ); }
   void setToFraig() const { _fraig = _globalFraig; }
   static void setFraig() { _globalFraig++; }
private:
   static unsigned _globalRef;
   mutable unsigned _ref;
   Var        _var;
   static unsigned _globalFraig;
   mutable unsigned _fraig;
protected:
   size_t _gateId;
   GateList _faninList;
   GateList _fanoutList;
   string _symbol;
   unsigned _lineNo;

   unsigned long long _simValue;
   static size_t _mask;

   unsigned _grpNo;
   vector< CirGate* > _fecGrp;

};

// _invert = true( should be inverted ) 
class CirUndefGate : public CirGate {
 public:
  CirUndefGate() {}
  CirUndefGate( size_t id, unsigned n ): 
    CirGate( id, n ), _type( UNDEF_GATE ) { _simValue = 0; }
  ~CirUndefGate() {}

  string getTypeStr() const { return "UNDEF"; }
  void printGate() const;
  GateType gateType() const { return _type; }
  size_t getLId() const { return 2 * _gateId; }
  bool invert( int index = 0 ) const { return 0; }//don't need

  void simulate();
 private:
  GateType _type;
};

class CirPiGate : public CirGate {
 public:
  CirPiGate() {}
  CirPiGate( size_t id, unsigned n ): 
    CirGate( id, n ), _type( PI_GATE ) {}
  ~CirPiGate() {}

  string getTypeStr() const { return "PI"; }
  void printGate() const;
  GateType gateType() const { return _type; }
  size_t getLId() const { return 2 * _gateId; }
  bool invert( int index = 0 ) const { return 0; }//don't need

  void setSim( unsigned long long s ) { _simValue = s; }
  void simulate();
 private:
  GateType _type;
};

class CirPoGate : public CirGate {
 public:
  CirPoGate() {}
  CirPoGate( size_t id, bool b, unsigned n ): 
    CirGate( id, n ), _invert( b ), _type( PO_GATE ) {}
  ~CirPoGate() {}

  string getTypeStr() const { return "PO"; }
  void printGate() const;
  GateType gateType() const { return _type; }
  size_t getLId() const { 
    if ( _invert ) return 2 * _faninList[ 0 ]->getId() + 1; 
    else           return 2 * _faninList[ 0 ]->getId();
  }
  bool invert( int index = 0 ) const { return _invert; }
  void inverseInvert( int index = 0 ) { _invert= !_invert; }

  void simulate();
 private:
  bool _invert;
  GateType _type;
};

class CirAigGate : public CirGate {
 public:
  CirAigGate() {}
  CirAigGate( size_t id, vector< bool > b, unsigned n ): 
    CirGate( id, n ), _invert( b ), _type( AIG_GATE ) {}
  ~CirAigGate() {}

  string getTypeStr() const { return "AIG"; }
  void printGate() const;
  GateType gateType() const { return _type; }
  size_t getLId() const { return 2 * _gateId; }
  bool invert( int index = 0 ) const { return _invert[ index ]; }
  void inverseInvert( int index = 0 ) { _invert[ index ] = !_invert[ index ]; }
  vector< bool > getInvertVector() { return _invert; }
  bool isAig() const{ return true; }
  void simulate();

 private:
  vector< bool > _invert;
  GateType _type;
};

// _invert = false( const = 0 )
class CirConstGate : public CirGate {
 public:
  CirConstGate() {}
  CirConstGate( size_t id, unsigned n ): 
    CirGate( id, n ), _type( CONST_GATE ) { _simValue = 0; }
  ~CirConstGate() {}

  string getTypeStr() const { return "CONST"; }
  void printGate() const;
  GateType gateType() const { return _type; }
  size_t getLId() const { return 0; }
  bool invert( int index = 0 ) const { return 0; }//don't need
  void simulate();

 private:
  GateType _type;
};
#endif // CIR_GATE_H
