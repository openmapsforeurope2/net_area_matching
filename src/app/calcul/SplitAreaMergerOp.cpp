// APP
#include <app/calcul/SplitAreaMergerOp.h>
#include <app/params/ThemeParameters.h>

// BOOST
#include <boost/progress.hpp>

// EPG
#include <epg/Context.h>
#include <epg/params/EpgParameters.h>
#include <epg/sql/tools/numFeatures.h>
#include <epg/sql/DataBaseManager.h>
#include <epg/tools/TimeTools.h>
#include <epg/tools/FilterTools.h>

//OME2
#include <ome2/geometry/tools/GetEndingPointsOp.h>


namespace app
{
    namespace calcul
    {
        ///
        ///
        ///
        SplitAreaMergerOp::SplitAreaMergerOp(
            bool verbose
        ) : 
            _verbose(verbose)
        {
            _init();
        }

        ///
        ///
        ///
        SplitAreaMergerOp::~SplitAreaMergerOp()
        {
            _shapeLogger->closeShape("sam_small_merged");
            _shapeLogger->closeShape("sam_same_id_merged");
        }

        ///
        ///
        ///
        void SplitAreaMergerOp::Compute(
			bool verbose
		) {
            SplitAreaMergerOp splitAreaMergerOp(verbose);
            splitAreaMergerOp._compute();
        }


        ///
        ///
        ///
        void SplitAreaMergerOp::_init()
        {
            //--
            _logger = epg::log::EpgLoggerS::getInstance();
            _logger->log(epg::log::INFO, "[START] initialization: " + epg::tools::TimeTools::getTime());

            //--
            _shapeLogger = epg::log::ShapeLoggerS::getInstance();
            _shapeLogger->addShape("sam_small_merged", epg::log::ShapeLogger::POLYGON);
            _shapeLogger->addShape("sam_same_id_merged", epg::log::ShapeLogger::POLYGON);

            //--
            epg::Context *context = epg::ContextS::getInstance();

            // epg parameters
            epg::params::EpgParameters const& epgParams = context->getEpgParameters();
            std::string const areaTableName = epgParams.getValue(AREA_TABLE).toString();
            std::string const idName = epgParams.getValue(ID).toString();
            std::string const geomName = epgParams.getValue(GEOM).toString();

            //--
            _fsArea = context->getDataBaseManager().getFeatureStore(areaTableName, idName, geomName);

            //--
            _logger->log(epg::log::INFO, "[END] initialization: " + epg::tools::TimeTools::getTime());
        };

        ///
        ///
        ///
        void SplitAreaMergerOp::_compute() const {

            bool mergingOccured = _computeEngine();

            while(mergingOccured) {
                mergingOccured = _computeEngine(true);
            }

        }

        ///
        ///
        ///
        double SplitAreaMergerOp::_getLength(
	        ign::geometry::Polygon const& poly
        ) const {
	        double douglasPeuckerDist = 15;

	        ign::geometry::LineString medialAxis = ome2::geometry::tools::GetEndingPointsOp::computeMedialAxis(poly, douglasPeuckerDist).second;

            return medialAxis.length();
        }

        ///
        ///
        ///
        double SplitAreaMergerOp::_getLength(
	        ign::geometry::MultiPolygon const& mp
        ) const {
            double length = 0;

            for (size_t i = 0 ; i < mp.numGeometries() ; ++i) {
                length += _getLength(mp.polygonN(i));
            }

            return length;
        }

        ///
        ///
        ///
        bool SplitAreaMergerOp::_computeEngine( bool mergeByNatId ) const {

            // epg parameters
            epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
            std::string const geomName = epgParams.getValue(GEOM).toString();

            //--
            app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
            double const areaThreshold = themeParameters->getValue(SAM_SMALL_AREA_THRESHOLD).toDouble();
            double const lengthThreshold = themeParameters->getValue(SAM_SMALL_AREA_LENGTH_THRESHOLD).toDouble();
            std::string const nationalIdName = themeParameters->getValue(NATIONAL_IDENTIFIER_NAME).toString();
			std::string const wTagName = themeParameters->getParameter(W_TAG).getValue().toString();

            // paquets de areas a merger
            std::vector<std::map<std::string, ign::feature::Feature>> vmAreas;

            std::map<double, ign::feature::Feature> mSortedSmallAreas;
            std::set<std::string> sSmallAreas;

            ign::feature::FeatureFilter filterArea(wTagName + " IS NOT NULL");
            int numFeatures = epg::sql::tools::numFeatures(*_fsArea, filterArea);

            boost::progress_display display1(numFeatures, std::cout, "[ SAM [1/3] searching small areas % complete ]\n");
            ign::feature::FeatureIteratorPtr itArea = _fsArea->getFeatures(filterArea);
            while (itArea->hasNext())
            {
                ++display1;

                ign::feature::Feature const& fArea = itArea->next();
                ign::geometry::MultiPolygon const& mp = fArea.getGeometry().asMultiPolygon();
                std::string areaId = fArea.getId();
                std::string const& countryCode = fArea.getAttribute(countryCodeName).toString();
                
                double area = mp.area();

                if( area > areaThreshold) continue;

                double length = _getLength(mp);

                if( length > lengthThreshold && countryCode.find("#") == std::string::npos ) continue;

                mSortedSmallAreas.insert(std::make_pair(area, fArea));
                sSmallAreas.insert(fArea.getId());
            }

            for ( std::map<double, ign::feature::Feature>::const_reverse_iterator rmit = mSortedSmallAreas.rbegin() ; rmit != mSortedSmallAreas.rend() ; ++rmit ) {
                //DEBUG
                std::string id = rmit->second.getId();
                _logger->log(epg::log::DEBUG, id);
                
                std::map<double, ign::feature::Feature> mNeighbours = _getNeighboursWithArea(rmit->second, filterArea); // TODO confirmer qu'on cherche les voisins "w_tag IS NOT NULL" et non country LIKE%#%

                if ( rmit->second.getAttribute(countryCodeName).toString().find("#") != std::string::npos ) {
                    //petite surface #, on elimine les candidats qui ne sont pas #
                    std::map<double, ign::feature::Feature>::const_iterator mit = mNeighbours.begin();
                    while ( mit != mNeighbours.end() ) {
                        if ( mit->second.getAttribute(countryCodeName).toString().find("#") == std::string::npos ) {
                            mit = mNeighbours.erase(mit);
                        } else {
                            ++mit;
                        }
                    }
                }

                if (mNeighbours.empty()) continue;

                _shapeLogger->writeFeature("sam_small_merged", rmit->second);

                std::string const& idNat = rmit->second.getAttribute(nationalIdName).toString();

                const ign::feature::Feature* feat2Merge = &mNeighbours.rbegin()->second;
                for( std::map<double, ign::feature::Feature>::const_reverse_iterator rmit2 = mNeighbours.rbegin() ; rmit2 != mNeighbours.rend() ; ++rmit2 ) {
                    if( rmit2->second.getAttribute(nationalIdName).toString() == idNat ) {
                        feat2Merge = &rmit2->second;
                        break;
                    }
                }

                _addAreas(rmit->second, *feat2Merge, vmAreas);
            }

            if (mergeByNatId) {

                boost::progress_display display2(numFeatures, std::cout, "[ SAM [2/3] gathering areas to merge % complete ]\n");
                ign::feature::FeatureIteratorPtr itArea2 = _fsArea->getFeatures(filterArea);
                while (itArea2->hasNext())
                {
                    ++display2;

                    ign::feature::Feature const& fArea = itArea2->next();
                    ign::geometry::MultiPolygon const& mp = fArea.getGeometry().asMultiPolygon();
                    std::string areaId = fArea.getId();
                    std::string identifier = fArea.getAttribute(nationalIdName).toString();
                    
                    if ( sSmallAreas.find(areaId) != sSmallAreas.end() ) continue;

                    std::vector<ign::feature::Feature> vNeighbours = _getNeighbours(fArea, filterArea); // TODO confirmer qu'on cherche les voisins "w_tag IS NOT NULL" et non country LIKE%#%

                    for (std::vector<ign::feature::Feature>::const_iterator vit = vNeighbours.begin() ; vit != vNeighbours.end() ; ++vit) {
                        std::string identifierNeighbour = vit->getAttribute(nationalIdName).toString();

                        if( identifierNeighbour == identifier ) {

                            _shapeLogger->writeFeature("sam_same_id_merged", *vit);
                            _shapeLogger->writeFeature("sam_same_id_merged", fArea);

                            _addAreas(*vit, fArea, vmAreas);
                        }
                    }
                }
            }

            _mergeGroups(vmAreas);

            boost::progress_display display3(vmAreas.size(), std::cout, "[ SAM [3/3] merging areas % complete ]\n");
            for (std::vector<std::map<std::string, ign::feature::Feature>>::iterator vit = vmAreas.begin() ; vit != vmAreas.end() ; ++vit) {
                ++display3;

                double maxArea = 0;
                ign::feature::Feature* maxFeatPtr = 0;
                ign::geometry::GeometryPtr resultingGeomPtr(new ign::geometry::MultiPolygon());
                for(std::map<std::string, ign::feature::Feature>::iterator mit = vit->begin() ; mit != vit->end() ; ++mit) {
                    ign::geometry::MultiPolygon const& mp = mit->second.getGeometry().asMultiPolygon();
                    
                    resultingGeomPtr.reset(resultingGeomPtr->Union(mp));
                    double area = mp.area();
                    if (area > maxArea) {
                        maxArea = area;
                        maxFeatPtr = &mit->second;
                    }
                    _fsArea->deleteFeature(mit->first);
                }

                maxFeatPtr->setGeometry(resultingGeomPtr->toMulti());
                _fsArea->createFeature(*maxFeatPtr);
            }

            return vmAreas.size() != 0;
        }

        ///
        ///
        ///
        void SplitAreaMergerOp::_mergeGroups(std::vector<std::map<std::string, ign::feature::Feature>> & vmAreas) const {
            for(int i = vmAreas.size()-1 ; i > 0 ; --i) {
                for(size_t j = 0 ; j < i ; ++j) {
                    bool found = false;
                    for( std::map<std::string, ign::feature::Feature>::const_iterator mit = vmAreas[i].begin() ; mit != vmAreas[i].end() ; ++mit) {
                        if (vmAreas[j].find(mit->first) != vmAreas[j].end()) {
                            vmAreas[j].insert(vmAreas[i].begin(), vmAreas[i].end());
                            vmAreas.erase(vmAreas.begin()+i);
                            found = true;
                            break;
                        }
                    }
                    if (found) break;
                }
            }
        }

        ///
        ///
        ///
        bool SplitAreaMergerOp::_addAreas(
            ign::feature::Feature const& feat1,
            ign::feature::Feature const& feat2,
            std::vector<std::map<std::string, ign::feature::Feature>> & vmAreas
        ) const {
            bool foundGroup = false;
            for(std::vector<std::map<std::string, ign::feature::Feature>>::iterator vit = vmAreas.begin() ; vit != vmAreas.end() ; ++vit) {

                if( vit->find(feat1.getId()) != vit->end() ) {
                    vit->insert(std::make_pair(feat2.getId(), feat2));
                    foundGroup = true;
                    break;
                }
                if( vit->find(feat2.getId()) != vit->end() ) {
                    vit->insert(std::make_pair(feat1.getId(), feat1));
                    foundGroup = true;
                    break;
                }
            }
            if(!foundGroup) {
                vmAreas.push_back(std::map<std::string, ign::feature::Feature>());
                vmAreas.back().insert(std::make_pair(feat1.getId(), feat1));
                vmAreas.back().insert(std::make_pair(feat2.getId(), feat2));
            }
            return foundGroup;
        }

        ///
        ///
        ///
        std::map<double, ign::feature::Feature> SplitAreaMergerOp::_getNeighboursWithArea(
            ign::feature::Feature const& fArea,
            ign::feature::FeatureFilter const& filterArea_
        ) const {
            std::map<double, ign::feature::Feature> mNeighbours;

            ign::geometry::MultiPolygon const& areaGeom = fArea.getGeometry().asMultiPolygon();
            std::string areaId = fArea.getId();

            // epg parameters
            epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
            std::string const geomName = epgParams.getValue(GEOM).toString();

            ign::feature::FeatureFilter filterArea = filterArea_;
            epg::tools::FilterTools::addAndConditions(filterArea, "ST_INTERSECTS(" + geomName + ", ST_SetSRID(ST_GeomFromText('" + areaGeom.toString() + "'),3035))");
            ign::feature::FeatureIteratorPtr itArea = _fsArea->getFeatures(filterArea);
            
            while (itArea->hasNext())
            {
                ign::feature::Feature const& fNeighbour = itArea->next();
                std::string idNeighbour = fNeighbour.getId();

                if(idNeighbour == areaId) continue;

                ign::geometry::MultiPolygon const& neighbourGeom = fNeighbour.getGeometry().asMultiPolygon();

                ign::geometry::GeometryPtr intersectionGeom(areaGeom.Intersection(neighbourGeom));

                if( intersectionGeom->isPoint() || intersectionGeom->isMultiPoint() || intersectionGeom->isNull() || intersectionGeom->isEmpty() ) continue;

                mNeighbours.insert(std::make_pair(fNeighbour.getGeometry().asMultiPolygon().area(), fNeighbour));
            }

            return mNeighbours;
        }

        ///
        ///
        ///
        std::vector<ign::feature::Feature> SplitAreaMergerOp::_getNeighbours(
            ign::feature::Feature const& fArea,
            ign::feature::FeatureFilter const& filterArea_
        ) const {
            std::vector<ign::feature::Feature> vNeighbours;

            ign::geometry::MultiPolygon const& areaGeom = fArea.getGeometry().asMultiPolygon();
            std::string areaId = fArea.getId();

            // epg parameters
            epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
            std::string const geomName = epgParams.getValue(GEOM).toString();

            ign::feature::FeatureFilter filterArea = filterArea_;
            epg::tools::FilterTools::addAndConditions(filterArea, "ST_INTERSECTS(" + geomName + ", ST_SetSRID(ST_GeomFromText('" + areaGeom.toString() + "'),3035))");
            ign::feature::FeatureIteratorPtr itArea = _fsArea->getFeatures(filterArea);
            
            while (itArea->hasNext())
            {
                ign::feature::Feature const& fNeighbour = itArea->next();
                if(fNeighbour.getId() == areaId) continue;

                vNeighbours.push_back(fNeighbour);
            }
            
            return vNeighbours;
        }
    }
}
