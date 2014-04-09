/* Distributed Directional Fast Multipole Method
   Copyright (C) 2014 Austin Benson, Lexing Ying, and Jack Poulson

 This file is part of DDFMM.

    DDFMM is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    DDFMM is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DDFMM.  If not, see <http://www.gnu.org/licenses/>. */
#ifndef _WAVE3D_HPP_
#define _WAVE3D_HPP_

#include "comobject.hpp"
#include "kernel3d.hpp"
#include "mlib3d.hpp"
#include "numtns.hpp"
#include "parvec.hpp"
#include "vec3t.hpp"

#include <algorithm>
#include <vector>

#define NUM_CHILDREN (8)
#define CHILD_IND1(x) ((x & 4) >> 2)
#define CHILD_IND2(x) ((x & 2) >> 1)
#define CHILD_IND3(x) ((x & 1))

enum {
    WAVE3D_PTS = 1,
    WAVE3D_LEAF = 2,
};

//---------------------------------------------------------------------------
class PtPrtn {
public:
    std::vector<int> _ownerinfo;
public:
    PtPrtn() {;}
    ~PtPrtn() {;}
    std::vector<int>& ownerinfo() { return _ownerinfo; }
    int owner(int key) {
#ifndef RELEASE
	CallStackEntry entry("PtPrtn::owner");
#endif
        CHECK_TRUE(key < _ownerinfo[_ownerinfo.size() - 1]);
        // Get the process which owns the current point
	std::vector<int>::iterator vi = lower_bound(_ownerinfo.begin(),
                                                    _ownerinfo.end(), key + 1);
        return (vi - _ownerinfo.begin()) - 1;
    }
};

//---------------------------------------------------------------------------
typedef std::pair<int, Index3> BoxKey; // level, offset_in_level

class BoxDat {
public:
    // TODO (Austin): Some of these should be private
    int _fftnum;
    int _fftcnt;
    //
    DblNumMat _extpos;   // positions of exact points (leaf level)
    CpxNumVec _extden;   // Exact densities  
    CpxNumVec _upeqnden; // Upward equivalent density
    CpxNumVec _extval;   // Exact potential value
    CpxNumVec _dnchkval; // Downward check potential

    int _tag;
    std::vector<int> _ptidxvec;
    //
    std::vector<BoxKey> _undeidxvec;  // U List
    std::vector<BoxKey> _vndeidxvec;  // V List
    std::vector<BoxKey> _wndeidxvec;  // W List
    std::vector<BoxKey> _xndeidxvec;  // X List
    std::vector<BoxKey> _endeidxvec;  // Boxes in near field
    // _fndeidxvec maps a direction to a vector of boxes that are in the
    // interaction list of this box in that direction
    std::map< Index3, std::vector<BoxKey> > _fndeidxvec;

    // Auxiliarly data structures for FFT
    CpxNumTns _upeqnden_fft;
    std::set<Index3> _incdirset;
    std::set<Index3> _outdirset;

    BoxDat(): _tag(0), _fftnum(0), _fftcnt(0) {;} //by default, no children
    ~BoxDat() {;}

    // Size of directional interaction list
    int DirInteractionListSize() {
        int num = 0;
        for (std::map< Index3, std::vector<BoxKey> >::iterator mi = _fndeidxvec.begin();
             mi != _fndeidxvec.end(); ++mi) {
  	    num += mi->second.size();
        }
        return num;
    }
    //
    int& tag() { return _tag; }
    std::vector<int>& ptidxvec() { return _ptidxvec; }
    //
    std::vector<BoxKey>& undeidxvec() { return _undeidxvec; }
    std::vector<BoxKey>& vndeidxvec() { return _vndeidxvec; }
    std::vector<BoxKey>& wndeidxvec() { return _wndeidxvec; }
    std::vector<BoxKey>& xndeidxvec() { return _xndeidxvec; }
    std::vector<BoxKey>& endeidxvec() { return _endeidxvec; }
    std::map< Index3, std::vector<BoxKey> >& fndeidxvec() { return _fndeidxvec; }
    //
    DblNumMat& extpos() { return _extpos; }
    CpxNumVec& extden() { return _extden; }
    CpxNumVec& upeqnden() { return _upeqnden; }
    CpxNumVec& extval() { return _extval; }
    CpxNumVec& dnchkval() { return _dnchkval; }
    //
    CpxNumTns& upeqnden_fft() { return _upeqnden_fft; }
    std::set<Index3>& incdirset() { return _incdirset; }
    std::set<Index3>& outdirset() { return _outdirset; }
    int& fftnum() { return _fftnum; }
    int& fftcnt() { return _fftcnt; }
};


#define BoxDat_Number 18
enum {
    BoxDat_tag = 0,
    BoxDat_ptidxvec = 1,
    //
    BoxDat_undeidxvec = 2,
    BoxDat_vndeidxvec = 3,
    BoxDat_wndeidxvec = 4,
    BoxDat_xndeidxvec = 5,
    BoxDat_endeidxvec = 6,
    BoxDat_fndeidxvec = 7,
    //
    BoxDat_extpos = 8,
    BoxDat_extden = 9,
    BoxDat_upeqnden = 10,
    BoxDat_extval = 11,
    BoxDat_dnchkval = 12,
    //
    BoxDat_upeqnden_fft = 13,
    BoxDat_incdirset = 14,
    BoxDat_outdirset = 15,
    BoxDat_fftnum = 16,
    BoxDat_fftcnt = 17,
};

class BoxPrtn {
public:
    IntNumTns _ownerinfo;
public:
    BoxPrtn() {;}
    ~BoxPrtn() {;}
    IntNumTns& ownerinfo() { return _ownerinfo; }
    int owner(BoxKey key) {
#ifndef RELEASE
	CallStackEntry entry("BoxPrtn::owner");
#endif
        int lvl = key.first;
        Index3 idx = key.second;
        int COEF = pow2(lvl) / _ownerinfo.m();
        idx = idx / COEF;
        return _ownerinfo(idx(0), idx(1), idx(2));
    }
};

//---------------------------------------------------------------------------
class BoxAndDirKey {
public:
    BoxAndDirKey(BoxKey boxkey, Index3 dir) : _boxkey(boxkey), _dir(dir) {}
    BoxAndDirKey() {}
    ~BoxAndDirKey() {}

    BoxKey _boxkey;
    Index3 _dir;

    inline bool operator==(const BoxAndDirKey& rhs) const {
        return _boxkey == rhs._boxkey && _dir == rhs._dir;
    }

    inline bool operator!=(const BoxAndDirKey& rhs) const {
        return !operator==(rhs);
    }

    inline bool operator>(const BoxAndDirKey& rhs) const {
        return _boxkey > rhs._boxkey ||
               (_boxkey == rhs._boxkey && _dir > rhs._dir);
    }

    inline bool operator<(const BoxAndDirKey& rhs) const {
        return _boxkey < rhs._boxkey ||
               (_boxkey == rhs._boxkey && _dir < rhs._dir);
    }

    inline bool operator>=(const BoxAndDirKey& rhs) const {
        return operator>(rhs) || operator==(rhs);
    }

    inline bool operator<=(const BoxAndDirKey& rhs) const {
        return operator<(rhs) || operator==(rhs);
    }
};

#define BoxAndDirKey_Number 2
enum {
    BoxAndDirKey_boxkey = 0,
    BoxAndDirKey_dir = 1,
};


//---------------------------------------------------------------------------
class BoxAndDirLevelPrtn {
public:
    BoxAndDirLevelPrtn() {}
    ~BoxAndDirLevelPrtn() {}
    
    std::vector<BoxAndDirKey> partition_;
    std::vector<BoxAndDirKey> end_partition_;  // for debugging

    // Return process that owns the key.
    int owner(BoxAndDirKey& key) {
#ifndef RELEASE
	CallStackEntry entry("BoxAndDirLevelPrtn::owner");
#endif
        int ind = std::lower_bound(partition_.begin(),
                                   partition_.end(), key) - partition_.begin();
        --ind;
        if (ind < static_cast<int>(partition_.size()) - 1 &&
            key == partition_[ind + 1]) {
	    ++ind;
	}
	CHECK_TRUE(key <= end_partition_[ind]);
        return ind;
    }
};

//---------------------------------------------------------------------------
// Boundary data
class BoxAndDirDat {
public:
    CpxNumVec _dirupeqnden;
    CpxNumVec _dirdnchkval;
public:
    BoxAndDirDat() {;}
    ~BoxAndDirDat() {;}
    // Directional upward equivalent density
    CpxNumVec& dirupeqnden() { return _dirupeqnden; }
    // Directional downward check value
    CpxNumVec& dirdnchkval() { return _dirdnchkval; }
};

#define BoxAndDirDat_Number 2
enum {
    BoxAndDirDat_dirupeqnden = 0,
    BoxAndDirDat_dirdnchkval = 1,
};

typedef ParVec<BoxAndDirKey, BoxAndDirDat, BoxAndDirLevelPrtn> LevelBoxAndDirVec;

class BoxAndDirPrtn{
public:
    IntNumTns _ownerinfo;
public:
    BoxAndDirPrtn() {;}
    ~BoxAndDirPrtn() {;}
    IntNumTns& ownerinfo() { return _ownerinfo; }
    int owner(BoxAndDirKey key) {
#ifndef RELEASE
	CallStackEntry entry("BoxAndDirPrtn::owner");
#endif
        int lvl = key._boxkey.first;
        Index3 idx = key._boxkey.second;
        int COEF = pow2(lvl) / _ownerinfo.m();
        idx = idx / COEF;
        return _ownerinfo(idx(0), idx(1), idx(2));
    }
};

//---------------------------------------------------------------------------
typedef std::pair< std::vector<BoxKey>, std::vector<BoxKey> > box_lists_t;
typedef std::map< double, std::vector<BoxKey> > ldmap_t;
typedef std::vector< std::vector<BoxAndDirKey> > level_hdkeys_t;
typedef std::vector< std::map<Index3, std::vector<BoxKey> > > level_hdkeys_map_t;

class Wave3d: public ComObject {
public:
    //-----------------------
    ParVec<int, Point3, PtPrtn>* _posptr;
    Kernel3d _kernel;
    int _ACCU;
    int _NPQ;
    Mlib3d* _mlibptr; // Read data in parallel and then send to other processors
    IntNumTns _geomprtn;
    //
    double _K;
    Point3 _ctr;
    int _ptsmax;
    int _maxlevel;
    //
    ParVec<BoxKey, BoxDat, BoxPrtn> _boxvec;
    ParVec<BoxAndDirKey, BoxAndDirDat, BoxAndDirPrtn> _bndvec;
    //
    CpxNumTns _denfft, _valfft;
    fftw_plan _fplan, _bplan;
    //
    static Wave3d* _self;
public:
    Wave3d(const std::string& p);
    ~Wave3d();
    //member access
    ParVec<int, Point3, PtPrtn>*& posptr() { return _posptr; }
    Kernel3d& kernel() { return _kernel; }
    int& ACCU() { return _ACCU; }
    int& NPQ() { return _NPQ; }
    Mlib3d*& mlibptr() { return _mlibptr; }
    IntNumTns& geomprtn() { return _geomprtn; }
    double& K() { return _K; }
    Point3& ctr() { return _ctr; }
    int& ptsmax() { return _ptsmax; }
    int& maxlevel() { return _maxlevel; }

    //main functions
    int setup(std::map<std::string, std::string>& opts);

    // Compute the potentials at the target points.
    int eval( ParVec<int, cpx, PtPrtn>& den, ParVec<int, cpx, PtPrtn>& val);

    // Compute the true solution and store the relative err in relerr.
    int check(ParVec<int, cpx, PtPrtn>& den, ParVec<int, cpx, PtPrtn>& val,
              IntNumVec& chkkeyvec, double& relerr);

    bool CompareBoxAndDirKey(BoxAndDirKey a, BoxAndDirKey b) {
        return BoxWidth(a._boxkey) < BoxWidth(b._boxkey);
    }

private:
    double width() { return _K; }
    //access information from BoxKey
    Point3 BoxCenter(BoxKey& curkey);
    double BoxWidth(BoxKey& curkey) { return _K / pow2(curkey.first); }
    bool IsCellLevelBox(const BoxKey& curkey) { return curkey.first == CellLevel(); }

    // Return the key of the parent box of the box corresponding to curkey.
    BoxKey ParentKey(BoxKey& curkey) {
	return BoxKey(curkey.first - 1, curkey.second / 2);
    }

    // Return the key of a child box of the box corresponding to curkey.
    // The index into the 8 children is given by idx
    BoxKey ChildKey(BoxKey& curkey, Index3 idx) {
        return BoxKey(curkey.first + 1, 2 * curkey.second + idx);
    }

    BoxDat& BoxData(BoxKey& curkey) { return _boxvec.access(curkey); }

    bool IsLeaf(BoxDat& curdat) { return curdat.tag() & WAVE3D_LEAF; }

    // Returns true iff curdat contains points.
    bool HasPoints(BoxDat& curdat) { return curdat.tag() & WAVE3D_PTS; }

    // Determine whether the box corresponding to curkey is owned
    // by this processor.
    bool OwnBox(BoxKey& curkey, int mpirank) {
        return _boxvec.prtn().owner(curkey) == mpirank;
    }

    // Return dimension of this problem.    
    int dim() { return 3; }

    // The level such that the box has width 1
    int UnitLevel() { return int(round(log(_K) / log(2))); } 

    // The level where the geometry is partitioned.
    int CellLevel() { return int(round(log(_geomprtn.m()) / log(2))); }

    Index3 nml2dir(Point3 nml, double W);

    // Compute the parent direction given the child direction.  If the child
    // direction is for a box B, then the parent direction is a direction
    // associated with all children boxes C of B.
    //
    // dir is the child direction
    Index3 ParentDir(Index3 dir);
    std::vector<Index3> ChildDir(Index3 dir);
    double Dir2Width(Index3 dir);

    int SetupTree();
    static int setup_Q1_wrapper(int key, Point3& dat, std::vector<int>& pids);
    static int setup_Q2_wrapper(BoxKey key, BoxDat& dat, std::vector<int>& pids);
    int setup_Q1(int key, Point3& dat, std::vector<int>& pids);
    int setup_Q2(BoxKey key, BoxDat& dat, std::vector<int>& pids);
    int SetupTreeLowFreqLists(BoxKey curkey, BoxDat& curdat);
    int SetupTreeHighFreqLists(BoxKey curkey, BoxDat& curdat);
    bool SetupTreeFind(BoxKey wntkey, BoxKey& reskey);
    bool SetupTreeAdjacent(BoxKey me, BoxKey yo);
    int RecursiveBoxInsert(std::queue< std::pair<BoxKey, BoxDat> >& tmpq);
    int P();

    // Functions for evaluation

    // Travel up the octree and visit the boxes in the low frequency regime.
    // Compute outgoing non-directional equivalent densities using M2M.
    //
    // W is the width of the box
    // srcvec is the list of all boxes owned by this processor of width W
    // reqboxset is filled with all boxes whose information this processor needs
    // for the low-frequency downward pass
    int EvalUpwardLow(double W, std::vector<BoxKey>& srcvec,
                      std::set<BoxKey>& reqboxset);

    int EvalDownwardLow(double W, std::vector<BoxKey>& trgvec);
    int LowFreqUpwardPass(ldmap_t& ldmap, std::set<BoxKey>& reqboxset);
    int LowFreqDownwardComm(std::set<BoxKey>& reqboxset);
    int LowFreqDownwardPass(ldmap_t& ldmap);

    int HighFreqPass(level_hdkeys_map_t& level_hdmap_out,
		     level_hdkeys_map_t& level_hdmap_inc);

    int EvalUpwardHigh(double W, Index3 dir, std::vector<BoxKey>& srcvec);
    int EvalDownwardHigh(double W, Index3 dir, std::vector<BoxKey>& trgvec,
                         std::vector<BoxKey>& srcvec);

    int ConstructMaps(ldmap_t& ldmap,
		      level_hdkeys_t& level_hdkeys_out,
		      level_hdkeys_t& level_hdkeys_inc,
		      level_hdkeys_map_t& level_hdmap_out,
                      level_hdkeys_map_t& level_hdmap_inc);
    int GatherDensities(std::vector<int>& reqpts, ParVec<int,cpx,PtPrtn>& den);
    
    int UListCompute(BoxDat& trgdat);
    int XListCompute(BoxDat& trgdat, DblNumMat& dcp, DblNumMat& dnchkpos,
                     CpxNumVec& dnchkval);
    int WListCompute(BoxDat& trgdat, double W, DblNumMat& uep);
    int VListCompute(BoxDat& trgdat, double W, int _P, Point3& trgctr,
                     DblNumMat& uep, DblNumMat& dcp, CpxNumVec& dnchkval,
                     NumTns<CpxNumTns>& ue2dc);

    int LowFrequencyM2M(BoxKey& srckey, BoxDat& srcdat, DblNumMat& uep,
                        DblNumMat& ucp, NumVec<CpxNumMat>& uc2ue,
                        NumTns<CpxNumMat>& ue2uc);
    int LowFrequencyM2L(double W, BoxKey& trgkey, BoxDat& trgdat, DblNumMat& dcp,
                        NumTns<CpxNumTns>& ue2dc, CpxNumVec& dneqnden, DblNumMat& uep,
                        NumVec<CpxNumMat>& dc2de);
    int LowFrequencyL2L(BoxKey& trgkey, BoxDat& trgdat, DblNumMat& dep,
                        NumTns<CpxNumMat>& de2dc, CpxNumVec& dneqnden);

    int HighFrequencyM2M(double W, BoxAndDirKey& bndkey,
                         NumVec<CpxNumMat>& uc2ue, NumTns<CpxNumMat>& ue2uc);
    int HighFrequencyM2L(double W, Index3 dir, BoxKey trgkey, BoxDat& trgdat,
                         DblNumMat& dcp, DblNumMat& uep);
    int HighFrequencyL2L(double W, Index3 dir, BoxKey trgkey,
                         NumVec<CpxNumMat>& dc2de, NumTns<CpxNumMat>& de2dc);

    // Add keys to reqbndset for high-frequency M2L computations
    // For every target box, add all the keys corresponding to
    // directional boundaries in the high-frequency interaction lists
    // of the target boxes.  dir specifies the direction of the boundaries.
    int HighFreqInteractionListKeys(Index3 dir, std::vector<BoxKey>& target_boxes,
                                    std::set<BoxAndDirKey>& reqbndset);

     // For all keys in level_keys, add the children keys to children_keys.
     int HighFreqChildrenKeys(double W,
			      std::map<Index3, std::vector<BoxKey> >& level_keys,
			      std::vector<BoxAndDirKey>& children_keys);

    // Tools for data distribution.
    void PartitionDirections(level_hdkeys_t& level_hdkeys_out,
			     level_hdkeys_t& level_hdkeys_inc,
			     std::vector<LevelBoxAndDirVec>& level_hf_vecs);
};

//-------------------
int serialize(const PtPrtn&, std::ostream&, const std::vector<int>&);
int deserialize(PtPrtn&, std::istream&, const std::vector<int>&);
//-------------------
int serialize(const BoxDat&, std::ostream&, const std::vector<int>&);
int deserialize(BoxDat&, std::istream&, const std::vector<int>&);
//-------------------
int serialize(const BoxAndDirDat&, std::ostream&, const std::vector<int>&);
int deserialize(BoxAndDirDat&, std::istream&, const std::vector<int>&);
//-------------------
int serialize(const BoxAndDirKey&, std::ostream&, const std::vector<int>&);
int deserialize(BoxAndDirKey&, std::istream&, const std::vector<int>&);

#endif  // _WAVE3D_HPP_
