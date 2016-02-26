/****************************************************************************
  FileName     [ myHashMap.h ]
  PackageName  [ util ]
  Synopsis     [ Define HashMap and Cache ADT ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2009-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef MY_HASH_MAP_H
#define MY_HASH_MAP_H

#include <vector>

using namespace std;

// TODO: Implement your own HashMap and Cache classes.

//-----------------------
// Define HashMap classes
//-----------------------
// To use HashMap ADT, you should define your own HashKey class.
// It should at least overload the "()" and "==" operators.
//
/*
template < class CirGate >
class HashKey
{
public:
    HashKey( vector< CirGate* > f, vector< bool > i ) {
      size_t in0 = reinterpret_cast< size_t >( f[ 0 ] ),
             in1 = reinterpret_cast< size_t >( f[ 1 ] );
      bool b1 = i[ 0 ], b2 = i[ 1 ];
      if ( in0 > in1 ) { SWAP( in0, in1); SWAP( b1, b2); }
      // digit = 2^( index ) - 1, addInvert = 2^( index )
      size_t index = 13, digit = 1, addInvert = 1;
      for ( int i = 0 ; i < ( index + 1 ) ; ++i ) digit = digit << 1;
      digit -= 1;
      for ( int i = 0 ; i < index ; ++i ) addInvert = addInvert << 1;
      in0 = in0 & digit;
      in1 = in1 & digit;
      if ( b1 ) in0 += addInvert;
      if ( b2 ) in1 += addInvert;
    }
    size_t operator() () const { return ( in0 * 10007 + in1 ); }
    bool operator == (const HashKey& k) const { return ( in0 == in1 ); }

private:
    size_t in0, in1;

    template< typename T >
    void SWAP( T& in0, T& in1) { T temp; temp = in0; in0 = in1; in1 = temp; }
};
*/
class HashKey {
 public:
  HashKey() {}
  HashKey( unsigned long long s ): _b1( false ), _b2( false ), _in0( 0 ), _value( s ) {
    if ( _value & 1 ) _in1 = ~_value;
    else _in1 = _value;
  }
  //HashKey( vector< unsigned long long > s ) { _in0 = s[ 0 ]; _in1 = s[ 1 ]; }
  HashKey( bool b1, bool b2, unsigned long long n1, unsigned long long n2 ) {
    if ( n1 > n2 ) { _in0 = n1; _in1 = n2; _b1  = b1; _b2  = b2; }
    else           { _in0 = n2; _in1 = n1; _b1  = b2; _b2  = b1; }
  }
  size_t operator() () const { return ( _in0 * 10007 + _in1 ); }
  bool operator == ( const HashKey& k ) const { 
    return ( ( _in0 == k._in0 ) && ( _in1 == k._in1 ) && ( _b1 == k._b1 ) && ( _b2 == k._b2 ) ); }
private:
  bool _b1, _b2;
  unsigned long long _in0, _in1, _value;
};

template <class HashKey, class HashData>
class HashMap
{
typedef pair<HashKey, HashData> HashNode;

public:
   HashMap() : _numBuckets(0), _buckets(0) {}
   HashMap(size_t b) : _numBuckets(0), _buckets(0) { init(b); }
   ~HashMap() { reset(); }

   // [Optional] TODO: implement the HashMap<HashKey, HashData>::iterator
   // o An iterator should be able to go through all the valid HashNodes
   //   in the HashMap
   // o Functions to be implemented:
   //   - constructor(s), destructor
   //   - operator '*': return the HashNode
   //   - ++/--iterator, iterator++/--
   //   - operators '=', '==', !="
   //
   // (_bId, _bnId) range from (0, 0) to (_numBuckets, 0)
   //
   class iterator
   {
      friend class HashMap<HashKey, HashData>;

   public:
      iterator(HashMap<HashKey, HashData>* h = 0, size_t b = 0, size_t bn = 0)
      : _hash(h), _bId(b), _bnId(bn) {}
      iterator(const iterator& i)
      : _hash(i._hash), _bId(i._bId), _bnId(i._bnId) {}
      ~iterator() {} // Should NOT delete HashData

      const HashNode& operator * () const { return (*_hash)[_bId][_bnId]; }
      HashNode& operator * () { return (*_hash)[_bId][_bnId]; }
      iterator& operator ++ () {
         if (_hash == 0) return (*this);
         if (_bId >= _hash->_numBuckets) return (*this);
         if (++_bnId >= (*_hash)[_bId].size()) {
            while ((++_bId < _hash->_numBuckets) && (*_hash)[_bId].empty());
            _bnId = 0;
         }
         return (*this);
      }
      iterator& operator -- () {
         if (_hash == 0) return (*this);
         if (_bnId == 0) {
            if (_bId == 0) return (*this);
            while ((*_hash)[--_bId].empty())
               if (_bId == 0) return (*this);
            _bnId = (*_hash)[_bId].size() - 1;
         }
         else
            --_bnId;
         return (*this);
      }
      iterator operator ++ (int) { iterator li=(*this); ++(*this); return li; }
      iterator operator -- (int) { iterator li=(*this); --(*this); return li; }

      iterator& operator = (const iterator& i) {
         _hash = i._hash; _bId = i._bId; _bnId = i._bnId; return (*this); }

      bool operator != (const iterator& i) const { return !(*this == i); }
      bool operator == (const iterator& i) const {
         return (_hash == i._hash && _bId == i._bId && _bnId == i._bnId); }

   private:
      HashMap<HashKey, HashData>*   _hash;
      size_t                        _bId;
      size_t                        _bnId;
   };

   void init(size_t b) {
      reset(); _numBuckets = b; _buckets = new vector<HashNode>[b]; }
   void reset() {
      _numBuckets = 0;
      if (_buckets) { delete [] _buckets; _buckets = 0; }
   }
   size_t numBuckets() const { return _numBuckets; }

   vector<HashNode>& operator [] (size_t i) { return _buckets[i]; }
   const vector<HashNode>& operator [](size_t i) const { return _buckets[i]; }

   // TODO: implement these functions
   //
   // Point to the first valid data
   iterator begin() const {
      if (_buckets == 0) return end();
/*
      size_t i = 0;
      while (_buckets[i].empty()) ++i;
      if (i == _numBuckets) return end();
      return iterator(const_cast<HashMap<HashKey, HashData>*>(this), i, 0);
*/
      for (size_t i = 0; i < _numBuckets; ++i)
         if (!_buckets[i].empty())
            return iterator(const_cast<HashMap<HashKey, HashData>*>(this), i, 0);
      return end();
   }
   // Pass the end
   iterator end() const {
      return iterator(const_cast<HashMap<HashKey, HashData>*>(this),
             _numBuckets, 0);
   }
   // return true if no valid data
   bool empty() const {
      for (size_t i = 0; i < _numBuckets; ++i)
         if (_buckets[i].size() != 0) return false;
      return true;
   }
   // number of valid data
   size_t size() const {
      size_t s = 0;
      for (size_t i = 0; i < _numBuckets; ++i) s += _buckets[i].size();
      return s;
   }

   // check if k is in the hash...
   // if yes, update n and return true;
   // else return false;
   bool check(const HashKey& k, HashData& n) const {
      size_t b = bucketNum(k);
      for (size_t i = 0, bn = _buckets[b].size(); i < bn; ++i)
         if (_buckets[b][i].first == k) {
            n = _buckets[b][i].second;
            return true;
         }
      return false;
   }

   // return true if inserted successfully (i.e. k is not in the hash)
   // return false is k is already in the hash ==> will not insert
   bool insert(const HashKey& k, const HashData& d) {
      size_t b = bucketNum(k);
      for (size_t i = 0, bn = _buckets[b].size(); i < bn; ++i)
         if (_buckets[b][i].first == k)
            return false;
      _buckets[b].push_back(HashNode(k, d));
      return true;
   }

   // return true if inserted successfully (i.e. k is not in the hash)
   // return false is k is already in the hash ==> still do the insertion
   bool replaceInsert(const HashKey& k, const HashData& d) {
      size_t b = bucketNum(k);
      for (size_t i = 0, bn = _buckets[b].size(); i < bn; ++i)
         if (_buckets[b][i].first == k) {
            _buckets[b][i].second = d;
            return false;
         }
      _buckets[b].push_back(HashNode(k, d));
      return true;
   }

   bool replace(const HashKey& k, const HashData& d) {
      size_t b = bucketNum(k);
      for (size_t i = 0, bn = _buckets[b].size(); i < bn; ++i)
         if (_buckets[b][i].first == k) {
            _buckets[b][i].second = d;
            return true;
         }
      return false;
   }

   // Need to be sure that k is not in the hash
   void forceInsert(const HashKey& k, const HashData& d) {
      _buckets[bucketNum(k)].push_back(HashNode(k, d)); }

private:
   // Do not add any extra data member
   size_t                   _numBuckets;
   vector<HashNode>*        _buckets;

   size_t bucketNum(const HashKey& k) const {
      return (k() % _numBuckets); }

};


//---------------------
// Define Cache classes
//---------------------
// To use Cache ADT, you should define your own HashKey class.
// It should at least overload the "()" and "==" operators.
//
// class CacheKey
// {
// public:
//    CacheKey() {}
//    
//    size_t operator() () const { return 0; }
//   
//    bool operator == (const CacheKey&) const { return true; }
//       
// private:
// }; 
// 
template <class CacheKey, class CacheData>
class Cache
{
typedef pair<CacheKey, CacheData> CacheNode;

public:
   Cache() : _size(0), _cache(0) {}
   Cache(size_t s) : _size(0), _cache(0) { init(s); }
   ~Cache() { reset(); }

   // NO NEED to implement Cache::iterator class

   // TODO: implement these functions
   //
   // Initialize _cache with size s
   void init(size_t s) { reset(); _size = s; _cache = new CacheNode[s]; }
   void reset() {  _size = 0; if (_cache) { delete [] _cache; _cache = 0; } }

   size_t size() const { return _size; }

   CacheNode& operator [] (size_t i) { return _cache[i]; }
   const CacheNode& operator [](size_t i) const { return _cache[i]; }

   // return false if cache miss
   bool read(const CacheKey& k, CacheData& d) const {
      size_t i = k() % _size;
      if (k == _cache[i].first) {
         d = _cache[i].second;
         return true;
      }
      return false;
   }
   // If k is already in the Cache, overwrite the CacheData
   void write(const CacheKey& k, const CacheData& d) {
      size_t i = k() % _size;
      _cache[i].first = k;
      _cache[i].second = d;
   }

private:
   // Do not add any extra data member
   size_t         _size;
   CacheNode*     _cache;
};


#endif // MY_HASH_H
