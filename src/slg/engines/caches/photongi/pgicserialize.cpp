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

#include "slg/engines/caches/photongi/photongicache.h"
#include "luxrays/utils/safesave.h"

using namespace std;
using namespace luxrays;
using namespace slg;

//------------------------------------------------------------------------------
// PhotonGICache serialization
//------------------------------------------------------------------------------

void PhotonGICache::LoadPersistentCache(const std::string &fileName) {
	SLG_LOG("Loading persistent PhotonGI cache: " + fileName);

	SerializationInputFile sif(fileName);

	sif.GetArchive() >> params;

	delete visibilityParticlesKdTree;
	visibilityParticlesKdTree = nullptr;
	visibilityParticles.clear();
	visibilityParticles.shrink_to_fit();

	sif.GetArchive() >> radiancePhotons;
	sif.GetArchive() >> radiancePhotonsBVH;
	sif.GetArchive() >> indirectPhotonTracedCount;

	sif.GetArchive() >> causticPhotons;
	sif.GetArchive() >> causticPhotonsBVH;
	sif.GetArchive() >> causticPhotonTracedCount;

	if (!sif.IsGood())
		throw runtime_error("Error while loading PhotonGI persistent cache: " + fileName);
	
}

void PhotonGICache::SavePersistentCache(const std::string &fileName) {
	SLG_LOG("Saving persistent PhotonGI cache: " + fileName);

	SafeSave safeSave(fileName);
	SerializationOutputFile sof(params.persistent.safeSave ? safeSave.GetSaveFileName() : fileName);

	sof.GetArchive() << params;

	sof.GetArchive() << radiancePhotons;
	sof.GetArchive() << radiancePhotonsBVH;
	sof.GetArchive() << indirectPhotonTracedCount;

	sof.GetArchive() << causticPhotons;
	sof.GetArchive() << causticPhotonsBVH;
	sof.GetArchive() << causticPhotonTracedCount;

	if (!sof.IsGood())
		throw runtime_error("Error while saving PhotonGI persistent cache: " + fileName);

	sof.Flush();
	
	if (params.persistent.safeSave)
		safeSave.Process();

	SLG_LOG("PhotonGI persistent cache saved: " << (sof.GetPosition() / 1024) << " Kbytes");
}

//------------------------------------------------------------------------------
// PhotonGICache serialization
//------------------------------------------------------------------------------

BOOST_CLASS_EXPORT_IMPLEMENT(slg::GenericPhoton)
BOOST_CLASS_EXPORT_IMPLEMENT(slg::VisibilityParticle)
BOOST_CLASS_EXPORT_IMPLEMENT(slg::Photon)
BOOST_CLASS_EXPORT_IMPLEMENT(slg::RadiancePhoton)
BOOST_CLASS_EXPORT_IMPLEMENT(slg::PhotonGICacheParams)
BOOST_CLASS_EXPORT_IMPLEMENT(slg::PhotonGICache)

template<class Archive> void PhotonGICache::serialize(Archive &ar, const u_int version) {
	ar & params;

	ar & visibilityParticles;
	ar & visibilityParticlesKdTree;

	ar & radiancePhotons;
	ar & radiancePhotonsBVH;
	ar & indirectPhotonTracedCount;

	ar & causticPhotons;	
	ar & causticPhotonsBVH;
	ar & causticPhotonTracedCount;
}

namespace slg {
// Explicit instantiations for portable archives
template void PhotonGICache::serialize(LuxOutputArchive &ar, const u_int version);
template void PhotonGICache::serialize(LuxInputArchive &ar, const u_int version);
// Explicit instantiations for polymorphic archives
template void PhotonGICache::serialize(boost::archive::polymorphic_oarchive &ar, const u_int version);
template void PhotonGICache::serialize(boost::archive::polymorphic_iarchive &ar, const u_int version);
}
