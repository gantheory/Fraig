/****************************************************************************
  FileName     [ cirMgr.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir manager functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstdio>
#include <ctype.h>
#include <cassert>
#include <cstring>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Implement memeber functions for class CirMgr

/*******************************/
/*   Global variable and enum  */
/*******************************/
CirMgr* cirMgr = 0;

enum CirParseError {
   EXTRA_SPACE,
   MISSING_SPACE,
   ILLEGAL_WSPACE,
   ILLEGAL_NUM,
   ILLEGAL_IDENTIFIER,
   ILLEGAL_SYMBOL_TYPE,
   ILLEGAL_SYMBOL_NAME,
   MISSING_NUM,
   MISSING_IDENTIFIER,
   MISSING_NEWLINE,
   MISSING_DEF,
   CANNOT_INVERTED,
   MAX_LIT_ID,
   REDEF_GATE,
   REDEF_SYMBOLIC_NAME,
   REDEF_CONST,
   NUM_TOO_SMALL,
   NUM_TOO_BIG,

   DUMMY_END
};

/**************************************/
/*   Static varaibles and functions   */
/**************************************/
static unsigned lineNo = 0;  // in printint, lineNo needs to ++
static unsigned colNo  = 0;  // in printing, colNo needs to ++
//static char buf[1024];
static string errMsg;
static int errInt;
static CirGate *errGate;

static bool
parseError(CirParseError err)
{
   switch (err) {
      case EXTRA_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Extra space character is detected!!" << endl;
         break;
      case MISSING_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing space character!!" << endl;
         break;
      case ILLEGAL_WSPACE: // for non-space white space character
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal white space char(" << errInt
              << ") is detected!!" << endl;
         break;
      case ILLEGAL_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal "
              << errMsg << "!!" << endl;
         break;
      case ILLEGAL_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal identifier \""
              << errMsg << "\"!!" << endl;
         break;
      case ILLEGAL_SYMBOL_TYPE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal symbol type (" << errMsg << ")!!" << endl;
         break;
      case ILLEGAL_SYMBOL_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Symbolic name contains un-printable char(" << errInt
              << ")!!" << endl;
         break;
      case MISSING_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing " << errMsg << "!!" << endl;
         break;
      case MISSING_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing \""
              << errMsg << "\"!!" << endl;
         break;
      case MISSING_NEWLINE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": A new line is expected here!!" << endl;
         break;
      case MISSING_DEF:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing " << errMsg
              << " definition!!" << endl;
         break;
      case CANNOT_INVERTED:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": " << errMsg << " " << errInt << "(" << errInt/2
              << ") cannot be inverted!!" << endl;
         break;
      case MAX_LIT_ID:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Literal \"" << errInt << "\" exceeds maximum valid ID!!"
              << endl;
         break;
      case REDEF_GATE:
         cerr << "[ERROR] Line " << lineNo+1 << ": Literal \"" << errInt
              << "\" is redefined, previously defined as "
              << errGate->getTypeStr() << " in line " << errGate->getLineNo()
              << "!!" << endl;
         break;
      case REDEF_SYMBOLIC_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ": Symbolic name for \""
              << errMsg << errInt << "\" is redefined!!" << endl;
         break;
      case REDEF_CONST:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Cannot redefine const (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_SMALL:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too small (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_BIG:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too big (" << errInt << ")!!" << endl;
         break;
      default: break;
   }
   return false;
}

/**************************************************************/
/*   class CirMgr member functions for circuit construction   */
/**************************************************************/
bool
CirMgr::readCircuit(const string& fileName)
{
  ifstream ifs( fileName.c_str() );
  //errorhandling 1.
  if ( ifs == 0 ) {
    cout << "Cannot open design \"" << fileName << "\"!!" << endl;
    return false;
  }
  string input;
  getline( ifs, input, '\n' );
  //get M, I, L, O, A, L should be 0
  //corresponding to miloa[ 0, 1, 2, 3, 4 ]
  vector< int > miloa = MILOA( input );
  lineNo++;
  if ( miloa.size() == 0 ) { clear(); return false; }
  //for create fanoutList
  vector< vector< unsigned > > allId;
  //store Literal ID of PI and AIG
  vector< vector< int > > allLId;
  _maxVariables = miloa[ 0 ];
  //for getGate() 
  for ( int i = 0 ; i < (  _maxVariables + miloa[ 3 ] + 1 + 100 ) ; ++i )
    _totalList.push_back( NULL );
  //create constGate
  createConstGate();
  //Primary Input
  for ( int i = 0 ; i < miloa[ 1 ] ; i++ ) {
    getline( ifs, input, '\n' );
    int lId;
    myStr2Int( input, lId );
    vector< int > storePILId;
    storePILId.push_back( lId );
    allLId.push_back( storePILId );
    if ( ( lId / 2 ) != 0 ) {
      CirGate* newGate = new CirPiGate( lId / 2, ++lineNo );
      _piList.push_back( newGate );
      _totalList[ lId / 2 ] = newGate;
    }
    else   
      _piList.push_back( getGate( 0 ) );
  }
  //Primary Output 
  //get LID, but not create CirPoGate
  vector< int > outputLId;
  unsigned outputId = _maxVariables + 1;
  for ( int i = 0 ; i < miloa[ 3 ] ; i++ ) {
    getline( ifs, input, '\n' );
    int lId;
    myStr2Int( input, lId);
    outputLId.push_back( lId );
    //create POGate
    CirGate* newGate = new CirPoGate( outputId, lId % 2, ++lineNo );
    _poList.push_back( newGate );
    _totalList[ outputId++ ] = newGate;

    vector< unsigned > forAllId;
    forAllId.push_back( lId / 2 );
    allId.push_back( forAllId );
  }
  //Aig
  //Create Gate and invert
  for ( int i = 0 ; i < miloa[ 4 ] ; i++ ) {
    getline( ifs, input, '\n' );
    //get Aigate id, input id 
    int num, flag = 0;
    vector< int > lId;
    string temp;
    vector< unsigned > forAllId;
    for ( int count = 0 ; count < 3 ; count++ ) {
      while ( ( input[ flag ] != ' ' ) && ( flag < input.size() ) ) 
        temp.push_back( input[ flag ++ ] );
      myStr2Int( temp, num );
      temp.clear();
      lId.push_back( num );
      forAllId.push_back( num / 2 );
      ++flag;
    }
    allId.push_back( forAllId );
    allLId.push_back( lId );
    //get invert
    vector< bool > invert;
    for ( int index = 1; index < 3 ; index++ )  
      invert.push_back( lId[ index ] % 2 );
    //create gate
    CirGate* newGate = new CirAigGate( lId[ 0 ] / 2, invert, ++lineNo );
    _aigList.push_back( newGate );
    _totalList[ lId[ 0 ] / 2 ] = newGate;
  }

  //create faninList
  for ( int i = 0 ; i < miloa[ 3 ] ; i++ ) {
    if ( getGate( outputLId[ i ] / 2 ) )
      _poList[ i ]->_faninList.push_back( getGate( outputLId[ i ] / 2 ) );
    //PO with floating fanin
    else {
      CirGate* newGate = new CirUndefGate( outputLId[ i ] / 2, 0 );
      _poList[ i ]->_faninList.push_back( newGate );
      _totalList[ outputLId[ i ] / 2 ] = newGate;
    }
  }

  int aigPos = 0;
  for ( int i = miloa[ 1 ] ; i < ( miloa[ 1 ] + miloa[ 4 ] ) ; i++ ) {
    GateList fanin;
    for ( int count = 1 ; count < 3 ; count++ ) {
      if ( getGate( allLId[ i ][ count ] / 2 ) )  
        fanin.push_back( getGate( allLId[ i ][ count ] / 2 ) );
      else {
        CirGate* newGate = new CirUndefGate( allLId[ i ][ count ]/2, 0 ); 
        _totalList[ allLId[ i ][ count ] / 2 ] = newGate;
        fanin.push_back( newGate );
      }
    }
    _aigList[ aigPos++ ]->_faninList = fanin;
  }
  
  //construct fanoutList vector< vector< unsiged > > allId
  createFanoutList( allId, miloa[ 3 ] );
  //create _netList, dfsTraversal
  dfsTraversal();
  //symbol section
  parseSymbol( ifs );
  return true;
}

void 
CirMgr::createFanoutList( vector< vector< unsigned > > allId, int outputNum ) {
  for ( int j = 0 ; j < allId.size() ; j ++ ) {
    if ( j < outputNum ) { 
      if ( getGate( allId[ j ][ 0 ] ) )
        setFanoutList( _maxVariables +1+j, getGate( allId[ j ][ 0 ] )->getId());
    }
    else { 
      for ( int count = 1 ; count < 3 ; count ++ )  
        setFanoutList( allId[ j ][ 0 ],getGate( allId[ j ][ count ] )->getId());
    }
  }
}

void 
CirMgr::parseSymbol( ifstream& ifs ) {
  string input;
  getline( ifs, input, '\n' );
  while ( input.size() > 0 ){
    _symbolInput.push_back( input );
    size_t index = input.find_first_of( " " );
    string number = input.substr( 1, index - 1 );
    string symbol = input.substr( index + 1, string::npos );

    int id;
    if ( myStr2Int( number, id ) ) {
      if ( input[ 0 ] == 'i' )   
        _piList[ id ]->setSymbolStr( symbol );
      else if ( input[ 0 ] == 'o' )  
        _poList[ id ]->setSymbolStr( symbol );
    }
    getline( ifs, input, '\n' );
  }
}

/**********************************************************/
/*   class CirMgr member functions for circuit printing   */
/**********************************************************/
/*********************
Circuit Statistics
==================
  PI          20
  PO          12
  AIG        130
------------------
  Total      162
*********************/
void
CirMgr::printSummary() const
{
  cout << endl;
  int piNum = 0, poNum = 0, aigNum = 0;
  for ( int i = 0 ; i < _totalList.size() ; ++i ) {
    if ( _totalList[ i ] ) {
      switch( _totalList[ i ]->gateType() ) { 
        case PI_GATE:
          ++piNum;
          break;
        case PO_GATE:
          ++poNum;
          break;
        case AIG_GATE:
          ++aigNum;
          break;
        default:
          break;
      }
    }
  }
  int sum = piNum + poNum + aigNum;
  cout << "Circuit Statistics" << endl;
  cout << "==================" << endl;
  cout << "  PI       " << setw( 5 ) << right <<  piNum << endl;
  cout << "  PO       " << setw( 5 ) << right <<  poNum << endl;
  cout << "  AIG      " << setw( 5 ) << right << aigNum << endl;
  cout << "------------------" << endl;
  cout << "  Total    " << setw( 5 ) << right <<    sum << endl;
  /* for eddy
  int count = 0;
  for ( size_t i = 0, in = _netList.size() ; i < in ; ++i ) 
    if ( _netList[ i]->gateType() == AIG_GATE )
      ++count;
  cout << "Aig in DFSList: " << count << endl;
  */
}

void
CirMgr::printNetlist() const
{
  cout << endl;
  for ( int i = 0 ; i < _netList.size() ; ++i ) {
    cout << "[" << i << "] ";
    _netList[ i ]->printGate();
  }
}

void
CirMgr::printPIs() const
{
   cout << "PIs of the circuit:";
   for ( int i = 0 ; i < _piList.size() ; ++i )  
     cout << " " << _piList[ i ]->getId();
   cout << endl;
}

void
CirMgr::printPOs() const
{
   cout << "POs of the circuit:";
   for ( int i = 0 ; i < _poList.size() ; ++i )  
     cout << " " << _poList[ i ]->getId();
   cout << endl;
}

void
CirMgr::printFloatGates() const
{
  //find floating gates
  vector< unsigned > floatingGate;
  for ( int i = 0 ; i < _totalList.size() ; ++i ) {
    if ( _totalList[ i ] ) {
      for ( int j = 0 ; j < _totalList[ i ]->faninSize() ; ++j ) { 
        if ( _totalList[ i ]->faninType( j ) == UNDEF_GATE ) { 
          floatingGate.push_back( _totalList[ i ]->getId() );
          break;
        }
      }
    }
  }
  if ( floatingGate.size() > 0 ) {
    cout << "Gates with floating fanin(s):";
    std::sort( floatingGate.begin(), floatingGate.end() );
    for ( int i = 0 ; i < floatingGate.size() ; ++i )
      cout << " " << floatingGate[ i ];
    cout << endl;
  }

  vector< unsigned > unusedGate;
  for ( int i = 0 ; i < _totalList.size() ; ++i ) {
    if ( _totalList[ i ] ) {
      if ( ( _totalList[ i ]->_fanoutList.size() == 0 ) && 
           ( _totalList[ i ]->gateType() != PO_GATE )   && 
           ( _totalList[ i ]->gateType() != CONST_GATE ) )
        unusedGate.push_back( _totalList[ i ]->getId() );
    }
  }

  if ( unusedGate.size() > 0 ) {
    cout << "Gates defined but not used  :";
    std::sort( unusedGate.begin(), unusedGate.end() );
    for ( int i = 0 ; i < unusedGate.size() ; ++i )
      cout << " " << unusedGate[ i ];
    cout << endl;
  }
}

void
CirMgr::printFECPairs() const
{
  vector< vector< unsigned > > fecPair;
  for ( int i = 0, in = _fecGrpList.size() ; i < in ; ++i ) {
    cout << "[" << i << "] ";
    size_t temp;
    for ( int j = 0, jn = _fecGrpList[ i ].size() ; j < jn ; ++j ) {
      if ( j == 0 ) temp = _fecGrpList[ i ][ j ]->getSim();
      if ( temp != _fecGrpList[ i ][ j ]->getSim() )
        cout << "!";
        cout << _fecGrpList[ i ][ j ]->getId();;
      if ( j != ( jn - 1 ) )
        cout << " ";
    }
    cout << endl;
  }
}

void
CirMgr::writeAag(ostream& outfile) const
{
  int aigNum = 0;
  for ( int i = 0 ; i < _netList.size() ; ++i ) 
    if ( _netList[ i ]->gateType() == AIG_GATE )
      ++aigNum;
  outfile << "aag " << _maxVariables 
          << " " << _piList.size() 
          << " 0"
          << " " << _poList.size() 
          << " " << aigNum << endl;
  for ( int i = 0 ; i < _piList.size() ; ++i ) 
    outfile << _piList[ i ]->getLId() << endl;
  for ( int i = 0 ; i < _poList.size() ; ++i )  
    outfile << _poList[ i ]->getLId() << endl;
  for ( int i = 0 ; i < _netList.size() ; ++i ) {
    if ( _netList[ i ]->gateType() == AIG_GATE ) {
      outfile << _netList[ i ]->getLId();
      for ( int j = 0 ; j < 2 ; ++j ) {
        if ( _netList[ i ]->invert( j ) )
          outfile << " " << _netList[ i ]->_faninList[ j ]->getLId() + 1;
        else 
          outfile << " " << _netList[ i ]->_faninList[ j ]->getLId();
      }
      outfile << endl;
    }
  }
  for ( int i = 0 ; i < _symbolInput.size() ; ++i )
    outfile << _symbolInput[ i ] << endl;
  outfile << 'c' << endl;
  outfile << "AAG output by Shun-Yao ( Gary ) Shih" << endl;
}

bool
comparePi( CirGate* a, CirGate* b ) { return ( a->getId() < b->getId() ); }
void
CirMgr::writeGate(ostream& outfile, CirGate *g) const
{
  vector< CirGate* > pi, aig;
  vector< string > symbol;
  CirGate::setGlobalRef();
  writedfs( g, pi, aig );
  outfile << "aag " << _maxVariables << " " << pi.size() << " 0 1 " << aig.size() << endl;;
  sort( pi.begin(), pi.end(), comparePi );
  for ( size_t i = 0, in = pi.size() ; i < in ; ++i ) 
    outfile << pi[ i ]->getId() * 2 << endl;
  outfile << g->getId() * 2 << endl;
  for ( size_t i = 0, in = aig.size() ; i < in ; ++i ) {
    outfile << aig[ i ]->getId() * 2;
    for ( size_t j = 0 ; j < 2 ; ++j ) {
      if ( aig[ i ]->invert( j ) )
        outfile << " " << aig[ i ]->_faninList[ j ]->getId() * 2 + 1;
      else 
        outfile << " " << aig[ i ]->_faninList[ j ]->getId() * 2;
    }
    outfile << endl;
  }
  for ( size_t i = 0, in = pi.size() ; i < in ; ++i )
    for ( size_t j = 0, jn = _piList.size() ; j < jn ; ++j )
      if ( _piList[ j ] == pi[ i ] )
        outfile << "i" << j << " " << pi[ i ]->getSymbol() << endl;
  outfile << "o0" << " " << g->getId() << endl;
  outfile << 'c' << endl;
  outfile << "Write gate (" << g->getId() << ") by Shun-Yao ( Gary ) Shih" << endl;
}

void 
CirMgr::writedfs( CirGate* g, vector< CirGate* >& pi, vector< CirGate* >& aig ) const {
  g->setToGlobalRef();
  for ( size_t i = 0 ; i < g->_faninList.size() ; i++ ) {
    if ( !( g->_faninList[ i ] )->isGlobalRef() && 
          ( g->_faninList[ i ] )->gateType() != UNDEF_GATE )
      writedfs( g->_faninList[ i ], pi, aig );
  }
  if ( g->gateType() == PI_GATE ) pi.push_back( g );
  else aig.push_back( g );
}
vector< int >
CirMgr::MILOA( const string& input ) {
  //errorhandling 2.
  vector< int > empty;
  myStrGetTok( input, errMsg );
  if ( errMsg != "aag" ) { 
    parseError( ILLEGAL_IDENTIFIER );
    return empty;
  }
  //if ( !detectExtraSpace( input ) ) return empty;
  //read Line 1.
  string temp;
  vector< int > miloa;
  int flag = 4, num;// M start from input[ 4 ]
  for ( int count = 0 ; count < 5 ; ++count ) { 
    myStrGetTok( input, temp, flag );
    if ( ( flag + temp.size() + 1 ) <= input.size() ) 
      flag += temp.size() + 1;
    else flag = input.size();
    colNo = flag;
    if ( myStr2Int( temp, num ) ) {
      temp.clear();
      miloa.push_back( num );
    }
    else {
      //errorhandling 3.
      if ( temp.size() == 0 ) {
        switch( count ) {
          case 0:  errMsg = "number of vars";    break;
          case 1:  errMsg = "number of PIs";     break;
          case 2:  errMsg = "number of latches"; break;
          case 3:  errMsg = "number of POs";     break;
          case 4:  errMsg = "number of AIGs";    break;
          default: errMsg = "numver of vars";    break;
        }
        parseError( MISSING_NUM );
        return empty;
      }
      else {
        errMsg = "number of vars(" + temp + ")";
        parseError( ILLEGAL_NUM );
        return empty;
      }
    }
  }
  if ( miloa[ 0 ] < ( miloa[ 1 ] + miloa[ 2 ] + miloa[ 4 ] ) ) {
    errMsg = "Num of variables";
    errInt = miloa[ 0 ];
    parseError( NUM_TOO_SMALL );
    return empty;
  }
  return miloa;
}

void 
CirMgr::dfsTraversal() {
  _netList.clear();
  CirGate::setGlobalRef();
  for ( int i = 0, in = _poList.size() ; i < in ; i++ ) {
    for ( int j = 0, jn = _poList[ i ]->faninSize() ; j < jn ; j++ ) {
      if ( ! (_poList[ i ]->_faninList[ j ])->isGlobalRef() && 
             (_poList[ i ]->_faninList[ j ])->gateType() != UNDEF_GATE )
        dfsTraversal( _poList[ i ]->_faninList[ j ] );
    }
    _netList.push_back( _poList[ i ] );
    _poList[ i ]->setToGlobalRef();
  }
}
void 
CirMgr::dfsTraversal( CirGate* child ) 
{
  child->setToGlobalRef();
  for ( int i = 0 ; i < child->_faninList.size() ; i++ ) {
    if ( ! (child->_faninList[ i ])->isGlobalRef() && 
           (child->_faninList[ i ])->gateType() != UNDEF_GATE )
      dfsTraversal( child->_faninList[ i ] );
  }
  _netList.push_back( child );
}

void 
CirMgr::setFanoutList( unsigned parent, unsigned child ) {
  CirGate* fanout = getGate( parent );
  CirGate* gate = getGate( child );
  gate->_fanoutList.push_back( fanout );
}

void 
CirMgr::createConstGate() {
  CirGate* newGate = new CirConstGate( 0, 0 );
  _totalList[ 0 ] = newGate;
}

void 
CirMgr::clear() {
   for ( int i = 0 ; i < _totalList.size() ; i ++ ) 
     if ( _totalList[ i ] )
       delete _totalList[ i ];
   vector< string >().swap( _symbolInput );
   _fecGrpList.clear();
}

