// APP
#include <app/calcul/PolygonSplitterOp.h>
#include <app/params/ThemeParameters.h>
#include <app/tools/zTools.h>

// BOOST
#include <boost/progress.hpp>

// EPG
#include <epg/Context.h>
#include <epg/params/EpgParameters.h>
#include <ome2/feature/sql/featureStorePostgisTools.h>
#include <epg/sql/DataBaseManager.h>
#include <epg/tools/StringTools.h>
#include <epg/tools/TimeTools.h>
#include <epg/tools/FilterTools.h>
#include <epg/tools/geometry/PolygonSplitter.h>


namespace app
{
    namespace calcul
    {
        ///
        ///
        ///
        PolygonSplitterOp::PolygonSplitterOp(
            std::string borderCode,
            bool verbose
        ) : 
            _countryCode(borderCode),
            _verbose(verbose)
        {
            _init();
        }

        ///
        ///
        ///
        PolygonSplitterOp::~PolygonSplitterOp()
        {
            std::map<std::string, epg::tools::geometry::SegmentIndexedGeometryInterface*>::const_iterator mit;
            for (mit= _mCountryCuttingIndx.begin() ; mit != _mCountryCuttingIndx.end() ; ++mit )
                delete mit->second;

            std::map<std::string, ign::geometry::MultiLineString*>::const_iterator mit2;
            for (mit2= _mCountryCuttingGeom.begin() ; mit2 != _mCountryCuttingGeom.end() ; ++mit2 )
                delete mit2->second;


            _shapeLogger->closeShape("ps_cutting_ls");
        }

        ///
        ///
        ///
        void PolygonSplitterOp::Compute(
			std::string borderCode, 
			bool verbose
		) {
            PolygonSplitterOp PolygonSplitterOp(borderCode, verbose);
            PolygonSplitterOp._compute();
        }

        ///
        ///
        ///
        void PolygonSplitterOp::_init()
        {
            //--
            _logger = epg::log::EpgLoggerS::getInstance();
            _logger->log(epg::log::INFO, "[START] initialization: " + epg::tools::TimeTools::getTime());

            //--
            _shapeLogger = epg::log::ShapeLoggerS::getInstance();
            _shapeLogger->addShape("ps_cutting_ls", epg::log::ShapeLogger::LINESTRING);

            //--
            epg::Context *context = epg::ContextS::getInstance();

            // epg parameters
            epg::params::EpgParameters const& epgParams = context->getEpgParameters();
            std::string const areaTableName = epgParams.getValue(AREA_TABLE).toString();
            std::string const idName = epgParams.getValue(ID).toString();
            std::string const geomName = epgParams.getValue(GEOM).toString();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
            
            // app parameters
            params::ThemeParameters *themeParameters = params::ThemeParametersS::getInstance();
            std::string const landmaskTableName = themeParameters->getValue(LANDMASK_TABLE).toString();
            std::string const landCoverTypeName = themeParameters->getValue(LAND_COVER_TYPE_NAME).toString();
            std::string const landAreaValue = themeParameters->getValue(LAND_COVER_TYPE_VALUE).toString();
            double const borderOffset = themeParameters->getValue(PS_BORDER_OFFSET).toDouble();
			std::string const inlandwaterValue = themeParameters->getValue(TYPE_INLAND_WATER).toString();

            //on recupere la geometry des pays
            std::vector<std::string> vCountry;
		    epg::tools::StringTools::Split(_countryCode, "#", vCountry);

            for (size_t i = 0 ; i < 2 ; ++i) {
                ign::geometry::MultiLineString* cuttingMlsPtr = new ign::geometry::MultiLineString();
                ign::feature::sql::FeatureStorePostgis* fsLandmask = context->getDataBaseManager().getFeatureStore(landmaskTableName, idName, geomName);
				ign::feature::FeatureIteratorPtr itLandmask = fsLandmask->getFeatures(ign::feature::FeatureFilter("(" + landCoverTypeName + " = '" + landAreaValue + "' OR " + landCoverTypeName + " = '" + inlandwaterValue + "') AND " + countryCodeName + " = '" + vCountry[i] + "'"));
				while (itLandmask->hasNext())
                {
                    ign::feature::Feature const& fLandmask = itLandmask->next();
                    ign::geometry::MultiPolygon const& mp = fLandmask.getGeometry().asMultiPolygon();

                    ign::geometry::GeometryPtr bufferGeom(mp.buffer(borderOffset));

                    _addLs(*bufferGeom, *cuttingMlsPtr);
                }
                _mCountryCuttingGeom.insert(std::make_pair(vCountry[i], cuttingMlsPtr));
                _mCountryCuttingIndx.insert(std::make_pair(vCountry[i], new epg::tools::geometry::SegmentIndexedGeometry(cuttingMlsPtr)));

                ign::feature::Feature feat;
                feat.setGeometry(*cuttingMlsPtr);
                _shapeLogger->writeFeature("ps_cutting_ls", feat);
            }

            //--
            _fsArea = context->getDataBaseManager().getFeatureStore(areaTableName, idName, geomName);

            //--
            _logger->log(epg::log::INFO, "[END] initialization: " + epg::tools::TimeTools::getTime());
        };

        ///
        ///
        ///
        void PolygonSplitterOp::_addLs(ign::geometry::Geometry const& geom, ign::geometry::MultiLineString & mls) const {
            ign::geometry::Geometry::GeometryType geomType = geom.getGeometryType();
            switch( geomType )
            {
                case ign::geometry::Geometry::GeometryTypeLineString :
                    {
                        ign::geometry::LineString const& ls = geom.asLineString();
                        mls.addGeometry(ls);
                        break;
                    }
                case ign::geometry::Geometry::GeometryTypeMultiLineString :
                    {
                        ign::geometry::MultiLineString const& mls_ = geom.asMultiLineString();
                        for (size_t i = 0 ; i < mls_.numGeometries() ; ++i ) 
                            if ( !mls_.lineStringN(i).isEmpty() )
                                _addLs(mls_.lineStringN(i), mls);
                        break;
                    }
                case ign::geometry::Geometry::GeometryTypePolygon :
                    {
                        ign::geometry::Polygon const& p = geom.asPolygon();
                        for (size_t i = 0 ; i < p.numRings() ; ++i ) 
                            if ( !p.ringN(i).isEmpty() )
                                _addLs(p.ringN(i), mls);
                        break;
                    }
                case ign::geometry::Geometry::GeometryTypeMultiPolygon :
                    {
                        ign::geometry::MultiPolygon const& mp = geom.asMultiPolygon();
                        for( size_t i = 0 ; i < mp.numGeometries() ; ++i )
                            _addLs(mp.polygonN(i), mls);
                        break;
                    }
                case ign::geometry::Geometry::GeometryTypeGeometryCollection :
                    {
                        ign::geometry::GeometryCollection const& collection = geom.asGeometryCollection();
                        for( size_t i = 0 ; i < collection.numGeometries() ; ++i )
                            _addLs(collection.geometryN(i), mls);
                        break;
                    }
                default:
                    break;
            }
        };

        ///
        ///
        ///
        void PolygonSplitterOp::_compute() const {
            // epg parameters
            epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();

            ign::feature::FeatureFilter filterArea;
            int numFeatures = ome2::feature::sql::numFeatures(*_fsArea, filterArea);
            boost::progress_display display(numFeatures, std::cout, "[ polygon splitter  % complete ]\n");

            ign::feature::FeatureIteratorPtr itArea = _fsArea->getFeatures(filterArea);
            while (itArea->hasNext())
            {
                ++display;
                
                ign::feature::Feature const& fArea = itArea->next();
                ign::geometry::MultiPolygon const& mp = fArea.getGeometry().asMultiPolygon();
                std::string idOrigin = fArea.getId();
                std::string countryCode = fArea.getAttribute(countryCodeName).toString();

                std::map<std::string, epg::tools::geometry::SegmentIndexedGeometryInterface*>::const_iterator mit = _mCountryCuttingIndx.find(countryCode);
                if (mit == _mCountryCuttingIndx.end()) {
                    _logger->log(epg::log::ERROR, "Unknown country [country code] " + countryCode);
                    return;
                }

                std::vector<ign::geometry::LineString> vLs;
                mit->second->getSegments( mp.getEnvelope(), vLs);
                ign::geometry::MultiLineString mls(vLs);

                std::vector< ign::geometry::Polygon > vPolygons;
                for (size_t i = 0 ; i < mp.numGeometries() ; ++i) {
                    ign::geometry::Polygon const& p = mp.polygonN(i);

                    if (!p.intersects(mls)) continue;

                    epg::tools::geometry::PolygonSplitter polySplitter(p);
                    polySplitter.addCuttingGeometry(mls);
                    polySplitter.split( vPolygons );
                }

                if (vPolygons.size() <= mp.numGeometries()) continue;

                for (size_t i = 0 ; i < vPolygons.size() ; ++i) {
                    ign::feature::Feature newFeat = fArea;

                    tools::zFiller(vPolygons[i], -1000); //TODO a parametrer
                    newFeat.setGeometry(vPolygons[i].toMulti());
                    _fsArea->createFeature(newFeat);
                }
                
                _fsArea->deleteFeature(idOrigin);
            }
        }
    }
}