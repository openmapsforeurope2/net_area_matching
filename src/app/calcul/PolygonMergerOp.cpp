// APP
#include <app/calcul/PolygonMergerOp.h>
#include <app/params/ThemeParameters.h>
#include <app/tools/zTools.h>

// BOOST
#include <boost/progress.hpp>

// EPG
#include <epg/Context.h>
#include <epg/params/EpgParameters.h>
#include <epg/sql/DataBaseManager.h>
#include <epg/tools/StringTools.h>
#include <epg/tools/TimeTools.h>
#include <epg/tools/FilterTools.h>

// SOCLE
#include <ign/geometry/algorithm/SnapOpGeos.h>


namespace app
{
    namespace calcul
    {
        ///
        ///
        ///
        PolygonMergerOp::PolygonMergerOp(
            bool verbose
        ) : 
            _verbose(verbose)
        {
            _init();
        }

        ///
        ///
        ///
        PolygonMergerOp::~PolygonMergerOp()
        {
            _shapeLogger->closeShape("pm_merged_area");
        }

        ///
        ///
        ///
        void PolygonMergerOp::Compute(
			bool verbose
		) {
            PolygonMergerOp PolygonMergerOp(verbose);
            PolygonMergerOp._compute();
        }

        ///
        ///
        ///
        void PolygonMergerOp::_init()
        {
            //--
            _logger = epg::log::EpgLoggerS::getInstance();
            _logger->log(epg::log::INFO, "[START] initialization: " + epg::tools::TimeTools::getTime());

            //--
            _shapeLogger = epg::log::ShapeLoggerS::getInstance();
            _shapeLogger->addShape("pm_merged_area", epg::log::ShapeLogger::POLYGON);

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
        void PolygonMergerOp::_compute() const {
            //--
            epg::Context *context = epg::ContextS::getInstance();

            // epg parameters
            epg::params::EpgParameters const& epgParams = context->getEpgParameters();
            std::string const areaTableName = epgParams.getValue(AREA_TABLE).toString();

            // app parameters
            params::ThemeParameters *themeParameters = params::ThemeParametersS::getInstance();
            std::string const natIdName = themeParameters->getValue(NATIONAL_IDENTIFIER_NAME).toString();

            std::list<std::string> lNatIds = _getAllIdentifiers(areaTableName);

            boost::progress_display display(lNatIds.size(), std::cout, "[ polygon merger  % complete ]\n");
            
            for( std::list<std::string>::const_iterator lit = lNatIds.begin(); lit != lNatIds.end() ; ++lit ) {
                ++display;

                ign::feature::FeatureIteratorPtr itArea = _fsArea->getFeatures(ign::feature::FeatureFilter(natIdName+"='"+*lit+"'"));

                ign::geometry::GeometryPtr mergedGeomPtr(new ign::geometry::MultiPolygon());
                ign::feature::Feature mergedFeat;
                std::set<std::string> sFeat2Delete;
                bool isFirst = true;
                while (itArea->hasNext())
                {
                    ign::feature::Feature const& fArea = itArea->next();
                    ign::geometry::MultiPolygon const& mp = fArea.getGeometry().asMultiPolygon();
                    std::string idOrigin = fArea.getId();

                    mergedGeomPtr.reset( ign::geometry::algorithm::SnapOpGeos::SnapTo( mp, *mergedGeomPtr, 0.1 ) ) ;

                    mergedGeomPtr.reset(mergedGeomPtr->Union(mp));

                    sFeat2Delete.insert(idOrigin);

                    if (isFirst) {
                        mergedFeat = fArea;
                        isFirst = false;
                    }
                }

                if (!mergedGeomPtr->isMultiPolygon() && !mergedGeomPtr->isPolygon()) {
                    _logger->log(epg::log::ERROR, "Unexpected geometry type for merged geometry : " + mergedGeomPtr->toString());
                    continue;
                }

                // on supprime les points avec un z == -1000, générés par le découpage
                tools::removePointWithZ(*mergedGeomPtr, -1000);

                if (mergedGeomPtr->isMultiPolygon()) {
                    ign::geometry::MultiPolygon const& mpResult = mergedGeomPtr->asMultiPolygon();

                    for ( size_t i = 0 ; i < mpResult.numGeometries() ; ++i) {
                        mergedFeat.setGeometry(mpResult.polygonN(i).toMulti());
                        _fsArea->createFeature(mergedFeat);
                    }
                } else {
                    mergedFeat.setGeometry(mergedGeomPtr->asPolygon().toMulti());
                    _fsArea->createFeature(mergedFeat);
                }

                for (std::set<std::string>::const_iterator sit = sFeat2Delete.begin(); sit != sFeat2Delete.end() ; ++sit)
                    _fsArea->deleteFeature(*sit);

            }
        }


        ///
        ///
        ///
        std::list<std::string> PolygonMergerOp::_getAllIdentifiers( std::string const& tableName ) const
        {
            std::list<std::string> lValues;

            // context
            epg::Context* context = epg::ContextS::getInstance();

            // app parameters
            params::ThemeParameters *themeParameters = params::ThemeParametersS::getInstance();
            std::string const fieldName = themeParameters->getValue(NATIONAL_IDENTIFIER_NAME).toString();

            std::string sql = "SELECT "+fieldName+", count(*) FROM "+tableName + " GROUP BY "+fieldName;

            ign::sql::SqlResultSetPtr	resultPtr = context->getDataBaseManager().getConnection()->query( sql );

            for( size_t i = 0 ; i < resultPtr->size() ; ++i )
                if (resultPtr->getFieldValue(i,1).toInteger() > 1) {
                    lValues.push_back( resultPtr->getFieldValue(i,0).toString() );
                }
			    
            return lValues;
        }
    }
}