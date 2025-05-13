// APP
#include <app/calcul/PolygonClippingOp.h>
#include <app/params/ThemeParameters.h>

// BOOST
#include <boost/progress.hpp>

// EPG
#include <epg/Context.h>
#include <epg/params/EpgParameters.h>
#include <ome2/feature/sql/NotDestroyedTools.h>
#include <epg/sql/DataBaseManager.h>
#include <epg/tools/StringTools.h>
#include <epg/tools/TimeTools.h>
#include <epg/tools/FilterTools.h>


namespace app
{
    namespace calcul
    {
        ///
        ///
        ///
        PolygonClippingOp::PolygonClippingOp(
            std::string countryCode,
            bool verbose
        ) : 
            _countryCode(countryCode),
            _verbose(verbose)
        {
            _init();
        }

        ///
        ///
        ///
        PolygonClippingOp::~PolygonClippingOp()
        {
            _shapeLogger->closeShape("pc_working_zone");
        }

        ///
        ///
        ///
        void PolygonClippingOp::Compute(
			std::string countryCode, 
			bool verbose
		) {
            PolygonClippingOp polygonClippingOp(countryCode, verbose);
            polygonClippingOp._compute();
        }

        ///
        ///
        ///
        void PolygonClippingOp::_init()
        {
            //--
            _logger = epg::log::EpgLoggerS::getInstance();
            _logger->log(epg::log::INFO, "[START] initialization: " + epg::tools::TimeTools::getTime());

            //--
            _shapeLogger = epg::log::ShapeLoggerS::getInstance();
            _shapeLogger->addShape("pc_working_zone", epg::log::ShapeLogger::POLYGON);

            //--
            epg::Context *context = epg::ContextS::getInstance();

            // epg parameters
            epg::params::EpgParameters const& epgParams = context->getEpgParameters();
            std::string const boundaryTableName = epgParams.getValue(TARGET_BOUNDARY_TABLE).toString();
            std::string const areaTableName = epgParams.getValue(AREA_TABLE).toString();
            std::string const idName = epgParams.getValue(ID).toString();
            std::string const geomName = epgParams.getValue(GEOM).toString();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
            
            // app parameters
            params::ThemeParameters *themeParameters = params::ThemeParametersS::getInstance();
            std::string const landmaskTableName = themeParameters->getValue(LANDMASK_TABLE).toString();
            std::string const landCoverTypeName = themeParameters->getValue(LAND_COVER_TYPE_NAME).toString();
            std::string const landAreaValue = themeParameters->getValue(TYPE_LAND_AREA).toString();
            double const landmaskBuffer = themeParameters->getValue(PC_LANDMASK_BUFFER).toDouble();
			std::string const inlandwaterValue = themeParameters->getValue(TYPE_INLAND_WATER).toString();

            // on recupere un buffer autour de la frontiere
            ign::geometry::GeometryPtr boundBuffPtr(new ign::geometry::Polygon());
            ign::feature::sql::FeatureStorePostgis* fsBoundary = context->getDataBaseManager().getFeatureStore(boundaryTableName, idName, geomName);
            ign::feature::FeatureIteratorPtr itBoundary = ome2::feature::sql::NotDestroyedTools::GetFeatures(*fsBoundary,ign::feature::FeatureFilter(countryCodeName +" = '"+_countryCode+"'"));
            while (itBoundary->hasNext())
            {
                ign::feature::Feature const& fBoundary = itBoundary->next();
                ign::geometry::LineString const& ls = fBoundary.getGeometry().asLineString();

                ign::geometry::GeometryPtr tmpBuffPtr(ls.buffer(100000));

                boundBuffPtr.reset(boundBuffPtr->Union(*tmpBuffPtr));
            }

            //on recupere la geometry des pays
            std::vector<std::string> vCountry;
		    epg::tools::StringTools::Split(_countryCode, "#", vCountry);

            for (std::vector<std::string>::iterator vit = vCountry.begin() ; vit != vCountry.end() ; ++vit) {
                ign::geometry::MultiPolygon mpLandmask;
                ign::feature::sql::FeatureStorePostgis* fsLandmask = context->getDataBaseManager().getFeatureStore(landmaskTableName, idName, geomName);

				ign::feature::FeatureIteratorPtr itLandmask = ome2::feature::sql::NotDestroyedTools::GetFeatures(*fsLandmask,ign::feature::FeatureFilter("(" + landCoverTypeName + " = '" + landAreaValue + "' OR " + landCoverTypeName + " = '" + inlandwaterValue + "') AND " + countryCodeName + " = '" + *vit + "'"));
				while (itLandmask->hasNext())
                {
                    ign::feature::Feature const& fLandmask = itLandmask->next();
                    ign::geometry::MultiPolygon const& mp = fLandmask.getGeometry().asMultiPolygon();
                    for (int i = 0; i < mp.numGeometries(); ++i)
                    {
                        mpLandmask.addGeometry(mp.polygonN(i));
                    }
                }

                //on calcul la geometry de travail
                _mCountryGeomWithBuffPtr.insert(std::make_pair(*vit, ign::geometry::GeometryPtr(boundBuffPtr->Intersection(mpLandmask)->buffer(landmaskBuffer))));

                ign::feature::Feature feat;
                feat.setGeometry(*_mCountryGeomWithBuffPtr[*vit]);
                _shapeLogger->writeFeature("pc_working_zone", feat);
            }

            //--
            _fsArea = context->getDataBaseManager().getFeatureStore(areaTableName, idName, geomName);

            //--
            _logger->log(epg::log::INFO, "[END] initialization: " + epg::tools::TimeTools::getTime());
        };

        ///
        ///
        ///
        void PolygonClippingOp::_compute() const {
            // epg parameters
            epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();

            ign::feature::FeatureIteratorPtr itArea = ome2::feature::sql::NotDestroyedTools::GetFeatures(*_fsArea,ign::feature::FeatureFilter(countryCodeName +" = '"+_countryCode+"'"));
            while (itArea->hasNext())
            {
                ign::feature::Feature const& fArea = itArea->next();
                ign::geometry::MultiPolygon const& mp = fArea.getGeometry().asMultiPolygon();
                std::string idOrigin = fArea.getId();

                std::map<std::string, ign::geometry::GeometryPtr>::const_iterator mit = _mCountryGeomWithBuffPtr.find(_countryCode);
                if (mit == _mCountryGeomWithBuffPtr.end()) {
                    _logger->log(epg::log::ERROR, "Unknown country [country code] " + _countryCode);
                    continue;
                }

                //TODO ne traiter que les polygones qui chevauchent le landmask
                if (mp.within(*mit->second)) continue;

                ign::geometry::GeometryPtr resultPtr(mit->second->Intersection(mp));
                ign::geometry::MultiPolygon mpResult;

                ign::geometry::Geometry::GeometryType geomType = resultPtr->getGeometryType();
                switch( geomType )
                {
                    case ign::geometry::Geometry::GeometryTypePolygon :
                        {
                            ign::geometry::Polygon const& p = resultPtr->asPolygon();
                            if ( !p.isEmpty() ) mpResult.addGeometry(p);
                            break;
                        }
                    case ign::geometry::Geometry::GeometryTypeMultiPolygon :
                        {
                            ign::geometry::MultiPolygon const& mp = resultPtr->asMultiPolygon();
                            for( size_t i = 0 ; i < mp.numGeometries() ; ++i )
                                if ( !mp.polygonN(i).isEmpty() ) mpResult.addGeometry(mp.polygonN(i));
                            break;
                        }
                    case ign::geometry::Geometry::GeometryTypeGeometryCollection :
                        {
                            ign::geometry::GeometryCollection const& collection = resultPtr->asGeometryCollection();
                            for( size_t i = 0 ; i < collection.numGeometries() ; ++i ) {
                                if( collection.geometryN(i).isPolygon() ) {
                                    ign::geometry::Polygon const& p = collection.geometryN(i).asPolygon();
                                    if ( !p.isEmpty() ) mpResult.addGeometry(p);
                                }
                                if( collection.geometryN(i).isMultiPolygon() ) {
                                    ign::geometry::MultiPolygon const& mp = collection.geometryN(i).asMultiPolygon();
                                    for( size_t i = 0 ; i < mp.numGeometries() ; ++i )
                                        if ( !mp.polygonN(i).isEmpty() ) mpResult.addGeometry(mp.polygonN(i));
                                }
                            }
                            break;
                        }
                }
                if (mpResult.numGeometries() && !mpResult.isEmpty() && !mpResult.isNull()) {
                    ign::feature::Feature newFeat = fArea;
                    newFeat.setGeometry(mpResult);
                    _fsArea->createFeature(newFeat);
                }
                
                _fsArea->deleteFeature(idOrigin);
            }
        } 
    }
}