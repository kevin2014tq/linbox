/* linbox/algorithms/lanczos.h
 * Copyright (C) 2002 LinBox
 *
 * Written by Bradford Hovinen <hovinen@cis.udel.edu>
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

#ifndef __LINBOX_lanczos_H
#define __LINBOX_lanczos_H

#include <vector>
#include <algorithm>

#include "linbox/blackbox/archetype.h"
#include "linbox/util/debug.h"
#include "linbox/vector/vector-domain.h"
#include "linbox/solutions/methods.h"

namespace LinBox
{

	/**
	  \brief Solve a linear system using the conjugate Lanczos iteration.
	 *
	 * Lanczos system solver class.
	 * This class encapsulates the functionality required for solving a linear
	 * system through the conjugate Lanczos iteration
	 */
	template <class Field, class Vector>
	class LanczosSolver {
	public:

		/** @brief Constructor
		 * @param F Field over which to operate
		 * @param traits @ref SolverTraits  structure describing user
		 *               options for the solver
		 */
		LanczosSolver (const Field &F, const Method::Lanczos &traits) :
			_traits (traits), _field (&F), _randiter (F), _VD (F)
		{}

		/** @brief Constructor with a random iterator
		 * @param F Field over which to operate
		 * @param traits @ref SolverTraits  structure describing user
		 *               options for the solver
		 * @param r Random iterator to use for randomization
		 */
		LanczosSolver (const Field &F, const Method::Lanczos &traits, typename Field::RandIter r) :
			_traits (traits), _field (&F), _randiter (r), _VD (F)
		{}

		/** Solve the linear system Ax = b.
		 *
		 * If the system is nonsingular, this method computes the unique
		 * solution to the system Ax = b. If the system is singular, it computes
		 * a random solution.
		 *
		 * If the matrix A is nonsymmetric, this method preconditions the matrix
		 * A with the preconditioner D_1 A^T D_2 A D_1, where D_1 and D_2 are
		 * random nonsingular diagonal matrices. If the matrix A is symmetric,
		 * this method preconditions the system with A D, where D is a random
		 * diagonal matrix.
		 *
		 * @param A Black box for the matrix A
		 * @param x Vector in which to store solution
		 * @param b Right-hand side of system
		 * @return Reference to solution vector
		 */
		template <class Blackbox>
		Vector &solve (const Blackbox &A, Vector &x, const Vector &b);

		inline const Field & field() const { return *_field; }
	private:

		// Run the Lanczos iteration and return the result. Return false
		// if the method breaks down. Do not check that Ax = b in the end
		template<class Blackbox>
		bool iterate (const Blackbox &A, Vector &x, const Vector &b);

		const Method::Lanczos &_traits;
		const Field                       *_field;
		typename Field::RandIter           _randiter;
		VectorDomain<Field>                _VD;

		Vector                    _w[2], _Aw; // Temporaries used in the Lanczos iteration
	};

}

#include "linbox/algorithms/lanczos.inl"

#endif // __LINBOX_lanczos_H

// Local Variables:
// mode: C++
// tab-width: 4
// indent-tabs-mode: nil
// c-basic-offset: 4
// End:
// vim:sts=4:sw=4:ts=4:et:sr:cino=>s,f0,{0,g0,(0,\:0,t0,+0,=s
