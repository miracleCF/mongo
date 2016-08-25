/**********************************************************************
 *
 * GEOS - Geometry Engine Open Source
 * http://geos.osgeo.org
 *
 * Copyright (C) 2006 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU Lesser General Public Licence as published
 * by the Free Software Foundation. 
 * See the COPYING file for more information.
 *
 **********************************************************************/

#ifndef GEOS_GEOM_COORDINATEARRAYSEQUENCEFACTORY_H
#define GEOS_GEOM_COORDINATEARRAYSEQUENCEFACTORY_H


#include <vector>

#include "CoordinateSequenceFactory.h" // for inheritance

#include "inline.h"

// Forward declarations
namespace geos {
	namespace geom { 
		class Coordinate;
	}
}

namespace geos {
namespace geom { // geos::geom

/**
 * \class CoordinateArraySequenceFactory geom.h geos.h
 *
 * \brief
 * Creates CoordinateSequences internally represented as an array of
 * Coordinates.
 */
class  CoordinateArraySequenceFactory: public CoordinateSequenceFactory {

public:
	CoordinateSequence *create() const;

	CoordinateSequence *create(std::vector<Coordinate> *coords, std::size_t dims=0) const;

   	/** @see CoordinateSequenceFactory::create(std::size_t, int) */
	CoordinateSequence *create(std::size_t size, std::size_t dimension=0) const;

	CoordinateSequence *create(const CoordinateSequence &coordSeq) const;

	/** \brief
	 * Returns the singleton instance of CoordinateArraySequenceFactory
	 */
	static const CoordinateSequenceFactory *instance();
};

/// This is for backward API compatibility
typedef CoordinateArraySequenceFactory DefaultCoordinateSequenceFactory;

} // namespace geos::geom
} // namespace geos

#ifdef GEOS_INLINE
# include "CoordinateArraySequenceFactory.inl"
#endif

#endif // ndef GEOS_GEOM_COORDINATEARRAYSEQUENCEFACTORY_H
