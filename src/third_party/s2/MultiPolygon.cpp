/**********************************************************************
 *
 * GEOS - Geometry Engine Open Source
 * http://geos.osgeo.org
 *
 * Copyright (C) 2011 Sandro Santilli <strk@keybit.net>
 * Copyright (C) 2001-2002 Vivid Solutions Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU Lesser General Public Licence as published
 * by the Free Software Foundation. 
 * See the COPYING file for more information.
 *
 **********************************************************************
 *
 * Last port: geom/MultiPolygon.java r320 (JTS-1.12)
 *
 **********************************************************************/

#include "Geometry.h"
#include "LineString.h"
#include "Polygon.h"
#include "MultiPolygon.h"
#include "MultiLineString.h"
#include "GeometryFactory.h"
#include "Dimension.h"

#include <cassert>
#include <string>
#include <vector>

#ifndef GEOS_INLINE
# include "MultiPolygon.inl"
#endif

using namespace std;

namespace geos {
namespace geom { // geos::geom

/*protected*/
MultiPolygon::MultiPolygon(vector<Geometry *> *newPolys, const GeometryFactory *factory)
	: Geometry(factory),
	  GeometryCollection(newPolys,factory)
{}

MultiPolygon::~MultiPolygon(){}

Dimension::DimensionType
MultiPolygon::getDimension() const {
	return Dimension::A; // area
}

int MultiPolygon::getBoundaryDimension() const {
	return 1;
}

string MultiPolygon::getGeometryType() const {
	return "MultiPolygon";
}

bool MultiPolygon::isSimple() const {
	return true;
}

Geometry* MultiPolygon::getBoundary() const {
	if (isEmpty()) {
		return getFactory()->createMultiLineString();
	}
	vector<Geometry *>* allRings=new vector<Geometry *>();
	for (size_t i = 0; i < geometries->size(); i++) {
		Polygon *pg=dynamic_cast<Polygon *>((*geometries)[i]);
		assert(pg);
		Geometry *g=pg->getBoundary();
		if ( LineString *ls=dynamic_cast<LineString *>(g) )
		{
			allRings->push_back(ls);
		}
		else
		{
			GeometryCollection* rings=dynamic_cast<GeometryCollection*>(g);
			for (size_t j=0, jn=rings->getNumGeometries();
					j<jn; ++j)
			{
				//allRings->push_back(new LineString(*(LineString*)rings->getGeometryN(j)));
				allRings->push_back(rings->getGeometryN(j)->clone());
			}
			delete g;
		}
	}

	Geometry *ret=getFactory()->createMultiLineString(allRings);
	//for (int i=0; i<allRings->size(); i++) delete (*allRings)[i];
	//delete allRings;
	return ret;
}

bool
MultiPolygon::equalsExact(const Geometry *other, double tolerance) const
{
    if (!isEquivalentClass(other)) {
      return false;
    }
	return GeometryCollection::equalsExact(other, tolerance);
}
GeometryTypeId
MultiPolygon::getGeometryTypeId() const {
	return GEOS_MULTIPOLYGON;
}

} // namespace geos::geom
} // namespace geos
