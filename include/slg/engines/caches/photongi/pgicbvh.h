/***************************************************************************
 * Copyright 1998-2018 by authors (see AUTHORS.txt)                        *
 *                                                                         *
 *   This file is part of LuxCoreRender.                                   *
 *                                                                         *
 * Licensed under the Apache License, Version 2.0 (the "License");         *
 * you may not use this file except in compliance with the License.        *
 * You may obtain a copy of the License at                                 *
 *                                                                         *
 *     http://www.apache.org/licenses/LICENSE-2.0                          *
 *                                                                         *
 * Unless required by applicable law or agreed to in writing, software     *
 * distributed under the License is distributed on an "AS IS" BASIS,       *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
 * See the License for the specific language governing permissions and     *
 * limitations under the License.                                          *
 ***************************************************************************/

#ifndef _SLG_PGICBVH_H
#define	_SLG_PGICBVH_H

#include <vector>

#include "slg/slg.h"
#include "slg/core/indexbvh.h"

namespace slg {

//------------------------------------------------------------------------------
// PGICPhotonBvh
//------------------------------------------------------------------------------

class Photon;
class NearPhoton;

class PGICPhotonBvh : public IndexBvh<Photon> {
public:
	PGICPhotonBvh(const std::vector<Photon> *entries, const u_int entryMaxLookUpCount,
			const float radius, const float normalAngle);
	virtual ~PGICPhotonBvh();

	u_int GetEntryMaxLookUpCount() const { return entryMaxLookUpCount; }
	float GetEntryNormalCosAngle() const { return entryNormalCosAngle; }
	
	void GetAllNearEntries(std::vector<NearPhoton> &entries,
			const luxrays::Point &p, const luxrays::Normal &n, const bool isVolume,
			float &maxDistance2) const;

	friend class boost::serialization::access;

private:
	// Used by serialization
	PGICPhotonBvh() { }

	template<class Archive> void serialize(Archive &ar, const u_int version) {
		ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(IndexBvh);
		ar & entryMaxLookUpCount;
		ar & entryNormalCosAngle;
	}

	u_int entryMaxLookUpCount;
	float entryNormalCosAngle;
};

//------------------------------------------------------------------------------
// PGICRadiancePhotonBvh
//------------------------------------------------------------------------------

class RadiancePhoton;

class PGICRadiancePhotonBvh : public IndexBvh<RadiancePhoton> {
public:
	PGICRadiancePhotonBvh(const std::vector<RadiancePhoton> *entries,
			const float radius, const float normalAngle);
	virtual ~PGICRadiancePhotonBvh();

	float GetEntryNormalCosAngle() const { return entryNormalCosAngle; }

	const RadiancePhoton *GetNearestEntry(const luxrays::Point &p, const luxrays::Normal &n,
			const bool isVolume) const;

	friend class boost::serialization::access;

private:
	// Used by serialization
	PGICRadiancePhotonBvh() { }

	template<class Archive> void serialize(Archive &ar, const u_int version) {
		ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(IndexBvh);
		ar & entryNormalCosAngle;
	}

	float entryNormalCosAngle;
};

}

BOOST_CLASS_VERSION(slg::PGICPhotonBvh, 1)
BOOST_CLASS_VERSION(slg::PGICRadiancePhotonBvh, 1)

BOOST_CLASS_EXPORT_KEY(slg::PGICPhotonBvh)
BOOST_CLASS_EXPORT_KEY(slg::PGICRadiancePhotonBvh)

#endif	/* _SLG_PGICBVH_H */
