/* -*- mode: c; style: linux -*- */

/* block.h
 * Copyright (C) 2000 Helix Code, Inc.
 *
 * Written by Bradford Hovinen <hovinen@helixcode.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef __BLOCK_H
#define __BLOCK_H

#include <gnome.h>

#include "math-object.h"

BEGIN_GNOME_DECLS

#define BLOCK(obj)          GTK_CHECK_CAST (obj, block_get_type (), Block)
#define BLOCK_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, block_get_type (), BlockClass)
#define IS_BLOCK(obj)       GTK_CHECK_TYPE (obj, block_get_type ())

typedef struct _Block Block;
typedef struct _BlockClass BlockClass;
typedef struct _BlockPrivate BlockPrivate;

typedef int (*BlockIteratorCB) (Block *block, MathObject *math_object,
				gpointer data);

struct _Block 
{
	MathObject parent;

	BlockPrivate *p;
};

struct _BlockClass 
{
	MathObjectClass math_object_class;

	void (*foreach) (Block *, BlockIteratorCB, gpointer);
};

guint block_get_type         (void);

/**
 * block_foreach:
 * @block: object
 * @callback: Callback to invoke on each object
 * @data: Data to pass to the callback on each invocation
 * 
 * Call the function @callback for each math object in the block, passing the
 * block, the math object, and any arbitrary data (given by the data
 * parameter) to the object. If the return value of the callback is nonzero,
 * return immediately; otherwise continue with the remaining objects.
 **/

void block_foreach           (Block *block, 
			      BlockIteratorCB callback, 
			      gpointer data);

END_GNOME_DECLS

#endif /* __BLOCK_H */
