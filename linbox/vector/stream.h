/* linbox/vector/stream.h
 * Copyright (C) 2002 Bradford Hovinen
 *
 * Written by Bradford Hovinen <hovinen@cis.udel.edu>
 *
 * ------------------------------------
 * 2003-02-03 Bradford Hovinen <bghovinen@math.uwaterloo.ca>
 *
 * RandomSparseStream::RandomSparseStream: put probability parameter before
 * vector dimension
 * ------------------------------------
 * 2002-09-05 Bradford Hovinen <bghovine@math.uwaterloo.ca>
 *
 *  - Renamed to stream.h and moved to linbox/vector
 *  - VectorFactory is now called VectorStream, which fits its purpose
 *    somewhat better
 *  - The interface is now closer to the interface for istream
 *  - RandomDenseVectorFactory, et al. are refactored into classes
 *    parameterized on the vector type and specialized appropriately. This
 *    allows, e.g. construction of a random dense vector in sparse
 *    representation and so on.
 *  - New constructor interface for RandomSparseStream accepts proportion of
 *    nonzero entries; removed existing constructors
 *  - Reindented, since the other changes are so numerous that diffs aren't a
 *    big deal anyway
 *
 * ------------------------------------
 * 2002-05-18 Bradford Hovinen <hovinen@cis.udel.edu>
 *
 * Refactor: Create one class StandardBasisFactory, parameterized by vector
 * type, with specializations for dense, sparse map, and sparse associative
 * vectors.
 *
 * ------------------------------------
 * Modified by Dmitriy Morozov <linbox@foxcub.org>. May 27, 2002.
 *
 * Added parametrization of the VectorCategroy tags by VectorTraits (see
 * vector-traits.h for more details).
 *
 * ------------------------------------
 *
 *
 * ========LICENCE========
 * This file is part of the library LinBox.
 *
 * LinBox is free software: you can redistribute it and/or modify
 * it under the terms of the  GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * ========LICENCE========
 *.
 */

/**
 * @file vector/stream.h
 *
 * @brief Generation of sequences of random vectors.
 *
 * Random, sparse, basis vectors,...
 */

#ifndef __LINBOX_vector_stream_H
#define __LINBOX_vector_stream_H


#include <givaro/givranditer.h>
#include "linbox/util/debug.h"
#include "linbox/randiter/mersenne-twister.h"
#include "linbox/vector/vector-traits.h"
#include <vector>
#include <cmath>


// stream
namespace LinBox
{

	/** \brief Vector factory.

	 * This is an abstract base class that generates a sequence of vectors
	 * in a generic way. Typical uses would be in tests, where the same test
	 * might be run on a sequence of random vectors or on e_1, ..., e_n.
	 \ingroup vector
	 */
	template <class _Vector>
	class VectorStream {
	public:
		typedef _Vector Vector;
		typedef VectorStream<Vector> Self_t;


		virtual ~VectorStream () {}

		/** Get the next vector from the factory and store it in v
		*/

		virtual Vector &get (Vector &v) = 0;

		/** Extraction operator form
		*/
		Self_t &operator >> (Vector &v)
		{ get (v); return *this; }

		/** Get the number of vectors to be constructed in this stream
		*/
		virtual size_t size () const = 0;

		/** Get the number of vectors constructed so far
		*/
		virtual size_t pos () const = 0;

		/** Get the dimension of each vector
		*/
		virtual size_t dim () const = 0;

		/** Return true if and only if the vector stream still has more vectors
		 * to construct
		 */
		virtual operator bool () const = 0;

		/** Reset the vector stream to the beginning.
		*/
		virtual void reset () = 0;

		/** Alias for reset
		*/
		void rewind () { reset (); }

		/** @name Legacy interface
		 * These functions provide compatiblity with older parts of the
		 * library. Their use is deprecated.
		 */

		//@{

		Vector &next (Vector &v) { return get (v); }
		size_t j () const { return pos (); }
		size_t m () const { return size (); }
		size_t n () const { return dim (); }

		//@}
	};

	/** @brief Constant vector factory.
	 * Returns the same vector repeatedly
	 */
	template <class _Vector>
	class ConstantVectorStream : public VectorStream<_Vector> {
	public:
		typedef _Vector Vector;
		typedef ConstantVectorStream<Vector> Self_t;

		/** Constructor.
		 * Construct a new factory with the given field and vector size.
		 * @param v Vector to return on next
		 * @param m Number of vectors to return (0 for unlimited)
		 */
		ConstantVectorStream (Vector &v, size_t m) :
		       	_v (v), _m (m), _j (0)
	       	{}

		/** Retrieve vector
		 * @param v Vector to use
		 */
		Vector &get (Vector &v)
		{
			if (_m == 0 || _j < _m)
				std::copy (_v.begin (), _v.end (), v.begin ());
#ifdef _LB_DEBUG
			else {
				std::cerr << "Vector stream: nothing to get" << std::endl;
			}
#endif
			return v; }

		/** Extraction operator form
		*/
		Self_t &operator >> (Vector &v)
		{ get (v); return *this; }
		/** Number of vectors to be created
		*/
		size_t size () const { return _m; }

		/** Number of vectors created so far
		*/
		size_t pos () const { return _j; }

		/** Dimension of the space
		*/
		size_t dim () const { return _v.size (); }

		/** Check whether we have reached the end
		*/
		operator bool () const { return _m == 0 || _j < _m; }

		/** Reset the factory to start at the beginning
		*/
		void reset () { _j = 0; }

	private:
		Vector &_v;
		size_t  _m;
		size_t  _j;
	};
}


// #include "linbox/vector/blas-vector.h"
// Dense
namespace LinBox
{
	template<class _Field, class _Rep> class BlasVector ;

	/** @brief Random dense vector stream.
	 * Generates a sequence of random dense vectors over a given field
	 */
	template <typename Field, typename _Vector = BlasVector<Field, typename Vector<Field>::Dense>,
              class RandIter = typename Field::RandIter,
              class Trait = typename VectorTraits<_Vector>::VectorCategory>
	class RandomDenseStream : public VectorStream<_Vector> {
	public:
		typedef _Vector Vector;
		typedef RandomDenseStream<Field, Vector, RandIter, Trait> Self_t;

		/** Constructor.
		 * Construct a new stream with the given field and vector size.
		 * @param F Field over which to create random vectors
		 * @param r
		 * @param n Size of vectors
		 * @param m Number of vectors to return (0 for unlimited)
		 */
		RandomDenseStream (const Field &F, RandIter &r, size_t n, size_t m = 0);

		/** Get next element
		 * @param v Vector into which to generate random vector
		 * @return reference to new random vector
		 */
		Vector &get (Vector &v);

		/** Extraction operator form
		*/
		Self_t &operator >> (Vector &v)
		{ get (v); return *this; }
		/** Number of vectors to be created
		*/
		size_t size () const;

		/** Number of vectors created so far
		*/
		size_t pos () const;

		/** Dimension of the space
		*/
		size_t dim () const;

		/** Check whether we have reached the end
		*/
		operator bool () const;

		/** Reset the stream to start at the beginning
		*/
		void reset ();
	};

	//! Specialization of random dense stream for dense vectors
	template <class Field, class _Vector, class RandIter>
	class RandomDenseStream<Field, _Vector, RandIter, VectorCategories::DenseVectorTag > : public VectorStream<_Vector> {
	public:
		typedef _Vector Vector;
		typedef RandomDenseStream<Field, Vector, RandIter, VectorCategories::DenseVectorTag > Self_t;

		RandomDenseStream (const Field &F, RandIter &r, size_t nn, size_t mm = 0) :
			_field (F), _r (r), _n (nn), _m (mm), _j (0)
		{}

		Vector &get (Vector &v)
		{
			typename Vector::iterator i;

			if ( (_m > 0) && (_j++ >= _m) )
				return v;

			for (i = v.begin (); i != v.end (); ++i)
				_r.random (*i);

			return v;
		}

		/** Extraction operator form
		*/
		Self_t &operator >> (Vector &v)
		{ get (v); return *this; }
		size_t size () const { return _m; }
		size_t pos () const { return _j; }
		size_t dim () const { return _n; }
		operator bool () const { return _m == 0 || _j < _m; }
		void reset () { _j = 0; }

	private:
		const Field &_field;
		RandIter     & _r;
		size_t       _n;
		size_t       _m;
		size_t       _j;
	};

}

#include "linbox/vector/sparse.h"
//Sparse
namespace LinBox
{
	/** @brief Random sparse vector stream.
	 * Generates a sequence of random sparse vectors over a given field
	 */
	template <class Field, class _Vector = Sparse_Vector<typename Field::Element>, class RandIter = typename Field::RandIter, class Trait = typename VectorTraits<_Vector>::VectorCategory>
	class RandomSparseStream : public VectorStream<_Vector> {
	public:
		typedef _Vector Vector;
		typedef RandomSparseStream<Field, Vector, RandIter, Trait > Self_t;

		/** Constructor.
		 * Construct a new stream with the given field and vector size.
		 * @param F Field over which to create random vectors
		 * @param r
		 * @param n Size of vectors
		 * @param p Proportion of nonzero entries
		 * @param m Number of vectors to return (0 for unlimited)
		 * @param seed
		 */
		RandomSparseStream (const Field &F, RandIter &r, double p, size_t n, size_t m = 0, uint64_t seed=(uint64_t)time (NULL));

		/** Get next element
		 * @param v Vector into which to generate random vector
		 * @return reference to new random vector
		 */
		Vector &get (Vector &v);

		/** Extraction operator form
		*/
		Self_t &operator >> (Vector &v)
		{ get (v); return *this; }
		/** Number of vectors to be created
		*/
		size_t size () const;

		/** Number of vectors created so far
		*/
		size_t pos () const;

		/** Dimension of the space
		*/
		size_t dim () const;

		/** Check whether we have reached the end
		*/
		operator bool () const;

		/** Reset the stream to start at the beginning
		*/
		void reset ();

		/** Set the probability of a nonzero entry
		*/
		void setP (double p);
	};

	//! Specialization of RandomSparseStream for dense vectors
	template <class Field, class _Vector, class RandIter>
	class RandomSparseStream<Field, _Vector, RandIter, VectorCategories::DenseVectorTag > : public VectorStream<_Vector> {
	public:
		typedef _Vector Vector;
		typedef RandomSparseStream<Field, Vector, RandIter, VectorCategories::DenseVectorTag > Self_t;

		RandomSparseStream (const Field &F, RandIter &r, double p, size_t n, size_t m = 0, uint64_t seed=(uint64_t)time (NULL)) :
			_field (F), _r1 (r), _r (_r1), _n (n), _p (p), _m (m), _j (0),
			MT (static_cast<uint32_t>(seed))
		{
			linbox_check ((p >= 0.0) && (p <= 1.0));
		}

		Vector &get (Vector &v)
		{

			if (_m > 0 && _j++ >= _m)
				return v;

			for (typename Vector::iterator i = v.begin (); i != v.end (); ++i) {
				double val;
				val = MT.randomDouble ();

				if (val < _p)
					_r.random (*i);
				else
					_field.assign (*i, _field.zero);
			}

			return v;
		}
		/** Extraction operator form
		*/
		Self_t &operator >> (Vector &v)
		{ get (v); return *this; }

		size_t size () const { return _m; }
		size_t pos () const { return _j; }
		size_t dim () const { return _n; }
		operator bool () const { return _m == 0 || _j < _m; }
		void reset () { _j = 0; }
		void setP (double p) { linbox_check ((p >= 0.0) && (p <= 1.0)); _p = p; }

	private:
		const Field                      &_field;
		RandIter                         & _r1;
        Givaro::GeneralRingNonZeroRandIter<Field, RandIter>  _r;
		size_t                            _n;
		double                            _p;
		size_t                            _m;
		size_t                            _j;
		MersenneTwister                   MT;
	};

	//! Specialization of RandomSparseStream for sparse sequence vectors
	template <class Field, class _Vector, class RandIter>
	class RandomSparseStream<Field, _Vector, RandIter, VectorCategories::SparseSequenceVectorTag > : public VectorStream<_Vector> {
	public:
		typedef RandomSparseStream<Field, _Vector, RandIter, VectorCategories::SparseSequenceVectorTag > Self_t;
		typedef _Vector Vector;

		RandomSparseStream (const Field &F, RandIter &r, double p,
				    size_t N, size_t M = 0, uint64_t seed= time (NULL)) :
                _field (F), 
                _r1 (r), 
                _r (r), 
                _n (N), 
                _p (p), 
                _m (M), 
                _j (0),
                MT (static_cast<uint32_t>(seed))
		{ setP (p); }

		Vector &get (Vector &v)
		{
			typename Field::Element x;
			size_t i = (size_t) -1;

			if (_m > 0 && _j++ >= _m)
				return v;

			v.clear ();

			while (1) {

				double val;
				int skip;

				val = MT.randomDouble ();
				skip = (int) (ceil (log (val) * _1_log_1mp));

				if (skip <= 0)
					++i;
				else
					i += (size_t)skip;

				if (i >= _n) break;

				_r.random (x);
				v.push_back (std::pair<size_t, typename Field::Element> (i, x));
			}

			return v;
		}
		/** Extraction operator form
		*/
		Self_t &operator >> (Vector &v)
		{ get (v); return *this; }

		size_t size () const { return _m; }
		size_t pos () const { return _j; }
		size_t dim () const { return _n; }
		operator bool () const { return _m == 0 || _j < _m; }
		void reset () { _j = 0; }

		void setP (double p)
		{
			linbox_check ((p >= 0.0) && (p <= 1.0));
			_p = p;
			_1_log_1mp   = 1 / log (1 - _p);
		}

	private:
		const Field                      &_field;
		RandIter                         & _r1;
		Givaro::GeneralRingNonZeroRandIter<Field, RandIter>  _r;
		size_t                            _n;
		double                            _p;
		double                            _1_log_1mp;
		size_t                            _m;
		size_t                            _j;
		MersenneTwister                   MT;
	};

	//! Specialization of RandomSparseStream for sparse associative vectors
	template <class Field, class _Vector, class RandIter>
	class RandomSparseStream<Field, _Vector, RandIter, VectorCategories::SparseAssociativeVectorTag > : public VectorStream<_Vector> {
	public:
		typedef _Vector Vector;
		typedef RandomSparseStream<Field, Vector, RandIter, VectorCategories::SparseAssociativeVectorTag > Self_t;

		RandomSparseStream (const Field &F, RandIter &r, double p, size_t N,
				    size_t M = 0, uint64_t seed= time (NULL)) :
			_field (F), _r1 (r), _r (_r1), _n (N), _k ((long) (p * N)), _j (0), _m (M),
			MT (static_cast<uint32_t>(seed))
		{}

		Vector &get (Vector &v)
		{
			typename Field::Element x;
			int i;

			if (_m > 0 && _j++ >= _m)
				return v;

			v.clear ();

			for (i = 0; i < _k; ++i) {
				size_t idx;
				_r.random (x);
				while (!_field.isZero (v[idx = MT.randomIntRange (0, (uint32_t)_n)])) ;
				v[idx] = x;
			}

			return v;
		}
		/** Extraction operator form
		*/
		Self_t &operator >> (Vector &v)
		{ get (v); return *this; }

		size_t size () const { return _m; }
		size_t pos () const { return _j; }
		size_t dim () const { return _n; }
		operator bool () const { return _m == 0 || _j < _m; }
		void reset () { _j = 0; }
		void setP (double p) { _k = (long) (p * _n); }

	private:
		const Field                      &_field;
		RandIter                         & _r1;
		Givaro::GeneralRingNonZeroRandIter<Field, RandIter> _r;
		size_t                            _n;
		long                              _k;
		size_t                            _j;
		size_t                            _m;
		MersenneTwister                   MT;
	};

	//! Specialization of RandomSparseStream for sparse parallel vectors
	template <class Field, class _Vector, class RandIter>
	class RandomSparseStream<Field, _Vector, RandIter, VectorCategories::SparseParallelVectorTag > : public VectorStream<_Vector> {
	public:
		typedef _Vector Vector;
		typedef RandomSparseStream<Field, Vector, RandIter, VectorCategories::SparseParallelVectorTag > Self_t;

		RandomSparseStream (const Field &F, RandIter &r, double p, size_t nn, size_t mm = 0, uint64_t  seed= time (NULL)) :
			_field (F), _r1 (r), _r (_r1), _n (nn), _m (mm), _j (0),
			MT (static_cast<uint32_t>(seed))
		{ setP (p); }

		Vector &get (Vector &v)
		{
			typename Field::Element x;
			size_t i = (size_t) -1;

			if (_m > 0 && _j++ >= _m)
				return v;

			v.first.clear ();
			v.second.clear ();

			while (1) {

				double val;
			int skip;

				val = MT.randomDouble ();
				skip = (int) (ceil (log (val) * _1_log_1mp));

				if (skip <= 0)
					++i;
				else
					i += (size_t) skip;

				if (i >= _n) break;

				_r.random (x);
				v.first.push_back (i);
				v.second.push_back (x);
			}

			return v;
		}

		/** Extraction operator form
		*/
		Self_t &operator >> (Vector &v)
		{ get (v); return *this; }

		size_t size () const { return _m; }
		size_t pos () const { return _j; }
		size_t dim () const { return _n; }
		operator bool () const { return _m == 0 || _j < _m; }
		void reset () { _j = 0; }

		void setP (double p)
		{
			linbox_check ((p >= 0.0) && (p <= 1.0));
			_p = p;
			_1_log_1mp   = 1 / log (1 - _p);
		}

	private:
		const Field                      &_field;
		RandIter                         & _r1;
        Givaro::GeneralRingNonZeroRandIter<Field, RandIter>  _r;
		size_t                            _n;
		double                            _p;
		double                            _1_log_1mp;
		size_t                            _m;
		size_t                            _j;
		MersenneTwister                   MT;
	};
}

// standard basis
namespace LinBox
{
	/** @brief Stream for \f$e_1,\cdots,e_n\f$.
	 * Generates the sequence (e_1,...,e_n) over a given field
	 *
	 * This class is generic with respect to the underlying vector
	 * representation.
	 */
	template <class Field, class _Vector, class Trait = typename VectorTraits<_Vector>::VectorCategory>
	class StandardBasisStream : public VectorStream<_Vector> {
	public:
		typedef _Vector Vector;
		typedef StandardBasisStream<Field, Vector, Trait > Self_t;

		/** Constructor.
		 * Construct a new stream with the given field and vector size.
		 * @param F Field over which to create vectors
		 * @param n Size of vectors
		 */
		StandardBasisStream (Field &F, size_t n);

		/** Get next element
		 * @param v Vector into which to generate vector
		 * @return reference to new vector
		 */
		Vector &get (Vector &v);

		/** Extraction operator form
		*/
		Self_t &operator >> (Vector &v)
		{ get (v); return *this; }
		/** Number of vectors to be created
		*/
		size_t size () const;

		/** Number of vectors created so far
		*/
		size_t pos () const;

		/** Dimension of the space
		*/
		size_t dim () const;

		/** Check whether we have reached the end
		*/
		operator bool () const;

		/** Reset the stream to start at the beginning
		*/
		void reset ();

	private:
		const Field              &_field;
		size_t                    _n;
		size_t                    _j;
	};

	//! Specialization of standard basis stream for dense vectors
	template <class Field, class _Vector>
	class StandardBasisStream<Field, _Vector, VectorCategories::DenseVectorTag > : public VectorStream<_Vector> {
	public:
		typedef _Vector Vector;
		typedef StandardBasisStream<Field, Vector, VectorCategories::DenseVectorTag > Self_t;

		StandardBasisStream (const Field &F, size_t N) :
			_field (F), _n (N), _j (0)
		{}

		Vector &get (Vector &v)
		{
			typename Vector::iterator i;

			size_t idx;
			for (i = v.begin (), idx = 0; i != v.end (); ++i, ++idx) {
				if (idx == _j)
					_field.assign(*i, _field.one);
				else
					_field.assign (*i, _field.zero);
			}

			++_j;

			return v;
		}

		/** Extraction operator form
		*/
		Self_t &operator >> (Vector &v)
		{
			get (v);
			return *this;
		}

		size_t size () const { return _n; }

		size_t pos () const { return _j; }

		size_t dim () const { return _n; }

		operator bool () const { return _j < _n; }

		void reset () { _j = 0; }

	private:
		const Field              &_field;
		size_t                    _n;
		size_t                    _j;
	};

	//! Specialization of standard basis stream for sparse sequence vectors
	template <class Field, class _Vector>
	class StandardBasisStream<Field, _Vector, VectorCategories::SparseSequenceVectorTag > : public VectorStream<_Vector> {
	public:
		typedef _Vector Vector;
		typedef StandardBasisStream<Field, Vector, VectorCategories::SparseSequenceVectorTag > Self_t;

		StandardBasisStream (Field &F, size_t N) :
			_field (F), _n (N), _j (0)
		{
		}

		Vector &get (Vector &v)
		{
			v.clear ();

			if (_j < _n)
				v.push_back (std::pair <size_t, typename Field::Element> (_j++, _field.one));

			return v;
		}
		/** Extraction operator form
		*/
		Self_t &operator >> (Vector &v)
		{ get (v); return *this; }

		size_t size () const { return _n; }
		size_t pos () const { return _j; }
		size_t dim () const { return _n; }
		operator bool () const { return _j < _n; }
		void reset () { _j = 0; }

	private:
		const Field              &_field;
		size_t                    _n;
		size_t                    _j;
	};

	//! Specialization of standard basis stream for sparse associative vectors
	template <class Field, class _Vector>
	class StandardBasisStream<Field, _Vector, VectorCategories::SparseAssociativeVectorTag > : public VectorStream<_Vector> {
	public:
		typedef _Vector Vector;
		typedef StandardBasisStream<Field, Vector, VectorCategories::SparseAssociativeVectorTag > Self_t;

		StandardBasisStream (Field &F, size_t N) :
			_field (F), _n (N), _j (0)
		{
		}

		Vector &get (Vector &v)
		{
			v.clear ();

			if (_j < _n)
				v.insert (std::pair <size_t, typename Field::Element> (_j++, _field.one));

			return v;
		}

		/** Extraction operator form
		*/
		Self_t &operator >> (Vector &v)
		{ get (v); return *this; }
		size_t pos () const { return _j; }
		size_t size () const { return _n; }
		size_t dim () const { return _n; }
		operator bool () const { return _j < _n; }
		void reset () { _j = 0; }

	private:
		const Field              &_field;
		size_t                    _n;
		size_t                    _j;
	};

	//! Specialization of standard basis stream for sparse parallel vectors
	template <class Field, class _Vector>
	class StandardBasisStream<Field, _Vector, VectorCategories::SparseParallelVectorTag > : public VectorStream<_Vector> {
	public:
		typedef _Vector Vector;
		typedef StandardBasisStream<Field, Vector, VectorCategories::SparseParallelVectorTag> Self_t;

		StandardBasisStream (Field &F, size_t N) :
			_field (F), _n (N), _j (0)
		{  }

		Vector &get (Vector &v)
		{
			v.first.clear ();
			v.second.clear ();

			if (_j < _n) {
				v.first.push_back (_j++);
				v.second.push_back (_field.one);
			}

			return v;
		}

		/** Extraction operator form
		*/
		Self_t &operator >> (Vector &v)
		{ get (v); return *this; }
		size_t size () const { return _n; }
		size_t pos () const { return _j; }
		size_t dim () const { return _n; }
		operator bool () const { return _j < _n; }
		void reset () { _j = 0; }

	private:
		const Field              &_field;
		size_t                    _n;
		size_t                    _j;
	};

} // namespace LinBox

#include "linbox/vector/stream-gf2.h"

#endif // __LINBOX_vector_stream_H

// Local Variables:
// mode: C++
// tab-width: 4
// indent-tabs-mode: nil
// c-basic-offset: 4
// End:
// vim:sts=4:sw=4:ts=4:et:sr:cino=>s,f0,{0,g0,(0,\:0,t0,+0,=s
