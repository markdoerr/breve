/*****************************************************************************
 *                                                                           *
 * The breve Simulation Environment                                          *
 * Copyright (C) 2000, 2001, 2002, 2003 Jonathan Klein                       *
 *                                                                           *
 * This program is free software; you can redistribute it and/or modify      *
 * it under the terms of the GNU General Public License as published by      *
 * the Free Software Foundation; either version 2 of the License, or         *
 * (at your option) any later version.                                       *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License for more details.                              *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with this program; if not, write to the Free Software               *
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *
 *****************************************************************************/

#include "config.h"

#if MACOSX
	#if IPHONE
        #include <OpenGLES/ES1/gl.h>
        #include <OpenGLES/ES1/glext.h>
	#else
		#include <GLUT/glut.h>
		#include <OpenGL/gl.h>
		#include <OpenGL/glu.h>
		#include <OpenGL/glext.h>
	#endif
#else
	#define GL_GLEXT_PROTOTYPES
	#include <GL/gl.h>
	#include <GL/glut.h>

	#if WINDOWS
		#include <GL/glext.h>
	#else
		#include <GL/glx.h>
		#include <GL/glext.h>
	#endif
#endif

#if HAVE_LIBOSMESA
	#include <GL/osmesa.h>
#endif
