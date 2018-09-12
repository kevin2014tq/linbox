/*
 * Copyright (C) 2004 Pascal Giorgi, Clément Pernet
 *               2013, 2014 the LinBox group
 *               2018 revamped by Pascal Giorgi 
 * Written by :
 *               Pascal Giorgi  pascal.giorgi@lirmm.fr
 *               Clément Pernet clement.pernet@imag.fr
 *               Brice Boyer (briceboyer) <boyer.brice@gmail.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * ========LICENCE========
 */

/*! @file matrix/densematrix/blas-matrix.h
 * @ingroup densematrix
 * A \c BlasMatrix<\c _Field > represents a matrix as an array of
 * <code>_Field::Element</code>s. It also has the BlasBlackbox interface.
 *
 */

#ifndef __LINBOX_matrix_densematrix_blas_matrix_H
#define __LINBOX_matrix_densematrix_blas_matrix_H

#include <linbox/linbox-config.h>
#include "linbox/util/debug.h"
#include "linbox/linbox-tags.h"
#include "linbox/vector/stream.h"
#include "linbox/vector/blas-vector.h"
#include "linbox/matrix/matrix-category.h"
#include "linbox/matrix/matrix-traits.h"
#include "linbox/util/matrix-stream.h"

#include "linbox/matrix/DenseMatrix/blas-transposed-matrix.h"
#include "linbox/matrix/matrix-domain.h"
#include "linbox/matrix/DenseMatrix/new-blas-submatrix.h"
namespace LinBox
{ /*  Blas Matrix */
	template<class Matrix>
	class MatrixDomain;

	/*! Dense matrix representation.
	 * @ingroup matrix
	 * A \p BlasMatrix is a matrix of \p _Field::Element, with the structure of BLAS matrices.
	 * It is basically a vector of \p _Field::Element.
	 * In the Mother model, a \p BlasMatrix is allocated by the user.
	 *@bug why not BlasMatrixDomain ?
	 */
	template <class _Field, class _Storage>
	class BlasMatrix {
	public:
		typedef _Field                                   Field;
		typedef typename Field::Element                Element;    //!< Element type
        typedef typename Field::Element_ptr        Element_ptr;    //!< Pointer to Element type
		typedef _Storage                               Storage;    //!< Actually a <code>std::vector<Element></code> (or alike: cstor(n), cstor(n, val), operator[], resize(n).)
		typedef BlasMatrix<Field,Storage>               Self_t;    //!< Self typeype
        typedef Self_t                              matrixType;    //!< matrix type
        typedef BlasSubmatrix< Self_t>           subMatrixType;
        typedef BlasSubmatrix<const Self_t> constSubMatrixType;
        
	protected:
		size_t			                               _row;
		size_t			                               _col;
		Storage			                               _rep;
		Element_ptr	 		                           _ptr;
        const Field&                                 _field;
        

		/*! @internal
		 * @name create BlasMatrix
		 * @{ */


		/*! @internal
		 * Copy data according to Matrix container structure (allow submatrix).*/
		template <class Matrix>
		void createBlasMatrix (const Matrix& A,const size_t i0,const size_t j0,const size_t m, const size_t n, MatrixContainerCategory::Container);
		template <class Matrix>
		void createBlasMatrix (const Matrix& A,const size_t i0,const size_t j0,const size_t m, const size_t n, MatrixContainerCategory::BlasContainer);
		template <class Matrix>
		void createBlasMatrix (const Matrix& A,const size_t i0,const size_t j0,const size_t m, const size_t n, MatrixContainerCategory::Blackbox);


		/*!@internal constructor from an iterator of elements.
		 * @param v iterator on to \c Element s */
        template <class constIterator>
		void createBlasMatrix (const constIterator& it) ;


		/*! @internal
		 * @}
		 */

	public:

        /////////////
		// MEMORY  //
		/////////////

		/// (Re)allocates a new \f$ m \times n\f$ zero matrix (shaped and ready).
		void init(const size_t & r = 0, const size_t & c = 0);

        
		/** Resize the matrix to the given dimensions.
		 * The state of the matrix's entries after a call to this method is
		 * undefined
		 * @param m Number of rows
		 * @param n Number of columns
		 * @param val
		 */
		void resize (const size_t &m, const size_t &n, const Element& val = Element()) ;


        //////////////////
		// CONSTRUCTORS //
		//////////////////

		/*! Allocates a new \f$ 0 \times 0\f$ matrix (shaped and ready).*/
		BlasMatrix (const _Field &F) ;


		/*! Allocates a new \f$ m \times n\f$ zero matrix (shaped and ready).
		 * @param F
		 * @param m rows
		 * @param n cols
		 */
		//@{
		BlasMatrix (const _Field &F, const size_t & m, const size_t &n) ;
		//@}

		/*! Constructor from a matrix stream.
		 * @param ms matrix stream.
		 */
		BlasMatrix(MatrixStream<_Field>& ms) ;

		/*! Generic copy constructor from either a blackbox or a matrix container.
		 * @param A matrix to be copied
		 */
		template <class Matrix>
		BlasMatrix (const Matrix &A) ;

		/*! Generic copy constructor from either a blackbox or a matrix container (allow submatrix).
		 * @param A matrix to be copied
		 * @param i0
		 * @param j0
		 * @param m rows
		 * @param n columns
		 */
		template <class Matrix>
		BlasMatrix (const Matrix& A, const size_t & i0, const size_t & j0, const size_t & m,  const size_t & n) ;
	 
		/** Constructor using a finite vector stream (stream of the rows).
		 * @param  F The field of entries; passed so that arithmetic may be done
		 *           on elements
		 * @param  stream A vector stream to use as a source of vectors for this
		 *                matrix
		 */
		template <class StreamVector>
		BlasMatrix (const Field &F, VectorStream<StreamVector> &stream) ;


		/*! Create a BlasMatrix from an iterator of elements
		 * @param F
		 * @param m
		 * @param n
		 * @param it
		 */
        template <class constIterator>
		BlasMatrix (const _Field &F, const size_t &m , const size_t &n, const constIterator& it) ;


        /*! Create a BlasMatrix from another matrix defined over a different field (use homomorphism if it exists)
		 * @param F Field of the created nmatrix
		 * @param A matrix to be copied
		 */
		template<class _Matrix>
		BlasMatrix  (const _Matrix &A, const _Field &F) ;
        
		/// Destructor.
		~BlasMatrix () {}

		//! operator = (copying data)
		Self_t& operator= (const Self_t& A) ;

		//! Rebind operator
		template<typename _Tp1>
		struct rebind ;

		/////////////////
		//  ACCESSORS  //
		/////////////////

		/** Get the number of rows in the matrix.
		 * @returns Number of rows in matrix
		 */
		size_t rowdim() const { return _row;}

		/** Get the number of columns in the matrix.
		 * @returns Number of columns in matrix
		 */
		size_t coldim() const { return _col;}

		/*! Get the stride of the matrix.*/		 
		size_t getStride() const { return _col;}

		/*! @internal
		 * Get read-only access to the matrix data.
		 */
	    Element_ptr getPointer() const { return _ptr;}
        Element_ptr& getWritePointer()  { return _ptr;}
        
		//////////////////
		//   ELEMENTS   //
		//////////////////


		/** Set the entry at the (i, j) position to a_ij.
		 * @param i Row number, 0...rowdim () - 1
		 * @param j Column number 0...coldim () - 1
		 * @param a_ij Element to set
		 */
		const Element& setEntry (size_t i, size_t j, const Element &a_ij)
        {
            field().assign(_rep[i*_col+j],a_ij); return _rep[i*_col+j]; // PG: this does not work with BitVector as _rep -> getting a Warning because BitVector operator[] does not return a reference but a boolean constructed on the fly (setEntry must return a reference [why ?])
        }

		/** Get a writeable reference to the entry in the (i, j) position.
		 * @param i Row index of entry
		 * @param j Column index of entry
		 * @returns Reference to matrix entry
		 */
		Element &refEntry (size_t i, size_t j)
        {
            return _rep[i*_col+j];
        }    

		/** Get a read-only reference to the entry in the (i, j) position.
		 * @param i Row index
		 * @param j Column index
		 * @returns Const reference to matrix entry
		 */
		const Element &getEntry (size_t i, size_t j) const
        {
            return _rep[i*_col+j];
        }

		/** Copy the (i, j) entry into x, and return a reference to x.
		 * This form is more in the Linbox style and is provided for interface
		 * compatibility with other parts of the library
		 * @param x Element in which to store result
		 * @param i Row index
		 * @param j Column index
		 * @returns Reference to x
		 */
		Element &getEntry (Element &x, size_t i, size_t j) const
        {
            field().assign(x,_rep[i*_col+j]); return x;
        }

        /** get a read only reference to the field of the matrix 
         */
		const _Field& field() const {return _field;}
        
        
		///////////////////
		//      I/O      //
		///////////////////

		/** Read the matrix from an input stream.
		 * The stream is in SMS, DENSE, or MatrixMarket format and is autodetected.
		 * @param file Input stream from which to read
		 */
		std::istream &read (std::istream &file);
        
		/** Write the matrix to an output stream.
		 * @param os Output stream to which to write
		 * @param f write in some format (@ref Tag::FileFormat::Format). Default is Maple's.
		 */
		std::ostream &write (std::ostream &os, LINBOX_enum (Tag::FileFormat) f = Tag::FileFormat::MatrixMarket) const;

        
		///////////////////
		//   ITERATORS   //
		///////////////////

		/** @name Column of rows iterator
		 * \brief
		 * The column of rows iterator traverses the rows of the
		 * matrix in ascending order. Dereferencing the iterator yields
		 * a row vector in dense format
		 */
		//@{
		typedef Subvector<typename Storage::iterator, typename Storage::const_iterator> Row;
		typedef Subvector<typename Storage::const_iterator>                    ConstRow;

		/*!  Row Iterator.
		 * @ingroup iterators
		 * @brief NO DOC
		 */
		class RowIterator;
		/*! Const Row Iterator.
		 * @ingroup iterators
		 * @brief NO DOC
		 */
		class ConstRowIterator;

		RowIterator      rowBegin ();
		RowIterator      rowEnd ();
		ConstRowIterator rowBegin () const;
		ConstRowIterator rowEnd   () const;
		//@}

		/** @name Row of columns iterator
		 * \brief
		 * The row of columns iterator traverses the columns of the
		 * matrix in ascending order. Dereferencing the iterator yields
		 * a column vector in dense format
		 */
		//@{
		typedef Subvector<Subiterator<typename Storage::iterator> >            Col;
		typedef Subvector<Subiterator<typename Storage::const_iterator> > ConstCol;
		typedef Col           Column;
		typedef ConstCol ConstColumn;

		/*! Col Iterator.
		 * @ingroup iterators
		 * @brief NO DOC
		 */
		class ColIterator;
		/*! Const Col Iterator.
		 * @ingroup iterators
		 * @brief NO DOC
		 */
		class ConstColIterator;

		ColIterator      colBegin ();
		ColIterator      colEnd ();
		ConstColIterator colBegin () const;
		ConstColIterator colEnd ()   const;
		//@}

		/** @name Iterator
		 * \brief
		 *
		 * The iterator is a method for accessing all entries in the matrix
		 * in some unspecified order. This can be used, e.g. to reduce all
		 * matrix entries modulo a prime before passing the matrix into an
		 * algorithm.
		 */
		//@{
		typedef typename Storage::iterator Iterator;
		typedef typename Storage::const_iterator ConstIterator;

		Iterator      Begin ();
		Iterator      End   ();
		ConstIterator Begin () const;
		ConstIterator End   () const;
		//@}

		/** @name Raw Indexed iterator
		 * \brief
		 *
		 * Like the raw iterator, the indexed iterator is a method for
		 * accessing all entries in the matrix in some unspecified order.
		 * At each position of the the indexed iterator, it also provides
		 * the row and column indices of the currently referenced entry.
		 * This is provided through it's \c rowIndex() and \c colIndex() functions.
		 */
		//@{
		class IndexedIterator;
		/*! Const Indexed Iterator.
		 * @ingroup iterators
		 * @brief NO DOC
		 */
		class ConstIndexedIterator;

		IndexedIterator      IndexedBegin ();
		IndexedIterator      IndexedEnd   ();
		ConstIndexedIterator IndexedBegin () const;
		ConstIndexedIterator IndexedEnd   () const;
		//@}

		/** Retrieve a reference to a row.
		 * Since rows may also be indexed, this allows A[i][j] notation
		 * to be used.
		 * @param i Row index
		 * @bug Rows and Cols should be BlasVectors
		 */
		//@{
		Row      operator[] (size_t i) ;
		ConstRow operator[] (size_t i) const ;
		//@}


		///////////////////////////////////////////
		//   SPARSE MATRIX  INTERFACE COMPLIANCY //
		///////////////////////////////////////////

		/** Compute column density.
         */
		template <class Vector>
		Vector &columnDensity (Vector &v) const
        {
            std::fill (v.begin (), v.end (), _row);   return v;         
        }
        /** Compute the number of non zero elt 
         */
		size_t size() const {return _row * _col;}
        /** Finalize some optimization in the matrix storage 
         */
        void finalize() {}

		///////////////////////////////////////
		//   BLACK BOX  INTERFACE COMPLIANCY //
		///////////////////////////////////////

		template <class Vector1, class Vector2>
		Vector1&  apply (Vector1& y, const Vector2& x) const
        {
            constSubMatrixType A(*this,0,0,rowdim(),coldim()); return A.apply(y,x);
        }
        
		template <class Vector1, class Vector2>
		Vector1&  applyTranspose (Vector1& y, const Vector2& x) const
        {
            constSubMatrixType A(*this,0,0,rowdim(),coldim()); return A.applyTranspose(y,x);
        }

		subMatrixType& applyRight(subMatrixType& Y, const subMatrixType& X)
        {
            subMatrixType A(*this); return A.applyRight(Y,X);
        }
        
		subMatrixType& applyLeft(subMatrixType& Y, const subMatrixType& X)
        {
            subMatrixType A(*this); return A.applyLeft(Y,X);
        }
        
        // init to field zero elements
		void zero() {
            subMatrixType B(*this, 0, 0, rowdim(), coldim());
			B.zero();            
        }

        // init to random field elements
		void random()
		{
			subMatrixType B(*this, 0, 0, rowdim(), coldim());
			B.random();
		}
       
	}; // end of class BlasMatrix


} // end of namespace LinBox


namespace LinBox
{ /* Triangular Matrix */
	//! Triangular BLAS matrix.
	template <class _Field, class _Storage >
	class TriangularBlasMatrix: public BlasMatrix<_Field,_Storage> {

	protected:

		LINBOX_enum (Tag::Shape)          _uplo; //!< upper or lower triangular
		LINBOX_enum (Tag::Diag)           _diag; //!< unit or non unit diagonal

	public:
		typedef _Field                       Field;
		typedef _Storage                         Rep;
		typedef typename Field::Element      Element;      //!< Element type
		typedef BlasMatrix<Field,Rep>           Father_t;
		typedef TriangularBlasMatrix<Field,Rep> Self_t;


		/*! Constructor for a new \c TriangularBlasMatrix.
		 * @param F
		 * @param m rows
		 * @param n cols
		 * @param y (non)unit diagonal
		 * @param x (upp/low)er matrix
		 */
		TriangularBlasMatrix (const Field & F,
                              const size_t m, const size_t n,
                              LINBOX_enum (Tag::Shape) x=Tag::Shape::Upper,
                              LINBOX_enum (Tag::Diag) y= Tag::Diag::NonUnit) ;

		/*! Constructor from a \c BlasMatrix (copy).
		 * @param A matrix
		 * @param y (non)unit diagonal
		 * @param x (upp/low)er matrix
		 */
		TriangularBlasMatrix (const BlasMatrix<Field,Rep>& A,
                              LINBOX_enum (Tag::Shape) x=Tag::Shape::Upper,
                              LINBOX_enum (Tag::Diag) y= Tag::Diag::NonUnit) ;

		/*! Constructor from a \c BlasMatrix (no copy).
		 * @param A matrix
		 * @param y (non)unit diagonal
		 * @param x (upp/low)er matrix
		 */
		TriangularBlasMatrix (BlasMatrix<Field,Rep>& A,
                              LINBOX_enum (Tag::Shape) x=Tag::Shape::Upper,
                              LINBOX_enum (Tag::Diag) y= Tag::Diag::NonUnit) ;

		/*! Constructor from a \c TriangularBlasMatrix (copy).
		 * @param A matrix
		 */
		TriangularBlasMatrix (const TriangularBlasMatrix<Field,Rep>& A) ;

		/*! Generic constructor from a \c Matrix (no copy).
		 * @param A matrix
		 * @param y (non)unit diagonal
		 * @param x (upp/low)er matrix
		 */
		template<class Matrix>
		TriangularBlasMatrix (const Matrix& A,
                              LINBOX_enum (Tag::Shape) x=Tag::Shape::Upper,
                              LINBOX_enum (Tag::Diag) y= Tag::Diag::NonUnit) ;

		/// get the shape of the matrix (upper or lower)
		LINBOX_enum (Tag::Shape) getUpLo() const ;

		/// Is the diagonal implicitly unit ?
		LINBOX_enum (Tag::Diag) getDiag() const ;

	}; // end of class TriangularBlasMatrix

} // LinBox

#include "linbox/matrix/DenseMatrix/new-blas-matrix.inl"
#include "linbox/matrix/DenseMatrix/blas-triangularmatrix.inl"

#endif // __LINBOX_densematrix_blas_matrix_H

// Local Variables:
// mode: C++
// tab-width: 4
// indent-tabs-mode: nil
// c-basic-offset: 4
// End:
// vim:sts=4:sw=4:ts=4:et:sr:cino=>s,f0,{0,g0,(0,\:0,t0,+0,=s
