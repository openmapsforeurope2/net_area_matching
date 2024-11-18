// APP
#include <app/calcul/GenerateIntersectionAreaOp.h>
#include <app/params/ThemeParameters.h>

// BOOST
#include <boost/progress.hpp>

// EPG
#include <epg/Context.h>
#include <epg/params/EpgParameters.h>
#include <epg/sql/tools/numFeatures.h>
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
		GenerateIntersectionAreaOp::GenerateIntersectionAreaOp(
			std::string borderCode,
			bool verbose
		) : 
			_borderCode(borderCode),
			_verbose(verbose)
		{
			_init();
		}

		///
		///
		///
		GenerateIntersectionAreaOp::~GenerateIntersectionAreaOp()
		{
		}

		///
		///
		///
		void GenerateIntersectionAreaOp::compute(
			std::string borderCode, 
			bool verbose
		) {
			GenerateIntersectionAreaOp op(borderCode, verbose);
			op._compute();
		}

		///
		///
		///
		void GenerateIntersectionAreaOp::_init()
		{
			epg::Context* context = epg::ContextS::getInstance();

			//--
			_logger = epg::log::EpgLoggerS::getInstance();

			//--
			std::string const areaTableName = context->getEpgParameters().getValue(AREA_TABLE).toString();
			std::string const idName = context->getEpgParameters().getValue(ID).toString();
			std::string const geomName = context->getEpgParameters().getValue(GEOM).toString();

			//--
			app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
			std::string intAreaTableName = themeParameters->getValue(INTERSECTION_AREA_TABLE).toString();
			if (intAreaTableName == "") {
				std::string const intAreaSuffix = themeParameters->getValue(INTERSECTION_AREA_TABLE_SUFFIX).toString();
				intAreaTableName = themeParameters->getParameter(AREA_TABLE_INIT).getValue().toString() + intAreaSuffix;
			}
			
			{
				std::ostringstream ss;
				ss << "DROP TABLE IF EXISTS " << intAreaTableName << " ;";
				ss << "CREATE TABLE " << intAreaTableName << "("
					<< idName << " uuid DEFAULT gen_random_uuid(),"
					<< geomName << " geometry(POLYGONZ),"
					<< ");";

				context->getDataBaseManager().getConnection()->update(ss.str());
			}

			//--
			_fsArea = context->getDataBaseManager().getFeatureStore(areaTableName, idName, geomName);

			//--
			_fsIntArea = context->getDataBaseManager().getFeatureStore(intAreaTableName, idName, geomName);

			//--
		    epg::tools::StringTools::Split(_borderCode, "#", _vCountry);

		};

		///
		///
		///
		void GenerateIntersectionAreaOp::_compute() const {

			// epg parameters
            epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
            std::string const idName = epgParams.getValue(ID).toString();

            size_t idCountryRef = 0;

            ign::feature::FeatureFilter filterArea(countryCodeName+"='"+_vCountry[idCountryRef]+"'");

            int numFeatures = epg::sql::tools::numFeatures(*_fsArea, filterArea);
            boost::progress_display display(numFeatures, std::cout, "[ computing area intersections % complete ]\n");

            ign::feature::FeatureIteratorPtr itArea = _fsArea->getFeatures(filterArea);

            while (itArea->hasNext())
            {
                ++display;

                ign::feature::Feature const& fArea = itArea->next();
                ign::geometry::MultiPolygon const& mp = fArea.getGeometry().asMultiPolygon();

                ign::geometry::GeometryPtr interAreaGeomPtr(_getIntersectingAreas(mp, idCountryRef));

				ign::geometry::GeometryPtr geomPtr(mp.Intersection(*interAreaGeomPtr));

				_persistGeom(*geomPtr);
            }
		}

		///
		///
		///
		ign::geometry::Geometry* GenerateIntersectionAreaOp::_getIntersectingAreas( ign::geometry::Geometry const& geom, size_t countryIndex ) const {
			 // epg parameters
            epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
            std::string const geomName = epgParams.getValue(GEOM).toString();
            std::string const idName = epgParams.getValue(ID).toString();

			ign::feature::FeatureFilter filterArea(countryCodeName+"='"+_vCountry[1-countryIndex]+"'");
			epg::tools::FilterTools::addAndConditions(filterArea, "ST_INTERSECTS(" + geomName + ", ST_SetSRID(ST_GeomFromText('" + geom.toString() + "'),3035))");

            ign::feature::FeatureIteratorPtr itArea = _fsArea->getFeatures(filterArea);

			ign::geometry::GeometryPtr areaUnionPtr(new ign::geometry::Polygon());

			while (itArea->hasNext())
			{
				ign::feature::Feature const& fArea = itArea->next();
				ign::geometry::MultiPolygon const& areaGeom = fArea.getGeometry().asMultiPolygon();

				areaUnionPtr.reset(areaUnionPtr->Union(areaGeom));
            }

			return areaUnionPtr.release();
		}

		///
		///
		///
		void GenerateIntersectionAreaOp::_persistGeom( ign::geometry::Geometry const& geom ) const {
			ign::geometry::Geometry::GeometryType geomType = geom.getGeometryType();
            switch( geomType )
            {
                case ign::geometry::Geometry::GeometryTypeNull :
                case ign::geometry::Geometry::GeometryTypePoint :
                case ign::geometry::Geometry::GeometryTypeMultiPoint :
                case ign::geometry::Geometry::GeometryTypeTriangle :
                case ign::geometry::Geometry::GeometryTypeTriangulatedSurface :
                case ign::geometry::Geometry::GeometryTypePolyhedralSurface :
				case ign::geometry::Geometry::GeometryTypeLineString :
				case ign::geometry::Geometry::GeometryTypeMultiLineString : 
					return;
                case ign::geometry::Geometry::GeometryTypePolygon :
					{
						ign::feature::Feature feat = _fsIntArea->newFeature();
						feat.setGeometry( geom.asPolygon() );
						_fsIntArea->createFeature(feat);
					}
                case ign::geometry::Geometry::GeometryTypeMultiPolygon :
                    {
						ign::geometry::MultiPolygon const& mp = geom.asMultiPolygon();
						for( size_t i = 0 ; i < mp.numGeometries() ; ++i ) {
                            _persistGeom( mp.polygonN(i) );
                        }
					}
                case ign::geometry::Geometry::GeometryTypeGeometryCollection :
                    {
                        ign::geometry::GeometryCollection const& collection = geom.asGeometryCollection();
                        for( size_t i = 0 ; i < collection.numGeometries() ; ++i ) {
                            _persistGeom( collection.geometryN(i) );
                        }
                    }
                default :
                    IGN_THROW_EXCEPTION( "Geometry type not allowed" );

			}
		}
	}
}