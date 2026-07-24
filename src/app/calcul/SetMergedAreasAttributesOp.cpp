// APP
#include <app/calcul/SetMergedAreasAttributesOp.h>
#include <app/params/ThemeParameters.h>
#include <app/tools/zTools.h>

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
#include <epg/tools/geometry/getArea.h>


namespace app
{
    namespace calcul
    {
        ///
        ///
        ///
        SetMergedAreasAttributesOp::SetMergedAreasAttributesOp(
            std::string borderCode,
            bool verbose
        ) : 
            _verbose(verbose),
            _borderCode(borderCode),
			_attrMerger(0)
        {
            _init();
        }

        ///
        ///
        ///
        SetMergedAreasAttributesOp::~SetMergedAreasAttributesOp()
        {
			if (_attrMerger)
				delete _attrMerger;
        }

        ///
        ///
        ///
        void SetMergedAreasAttributesOp::Compute(
            std::string borderCode,
			bool verbose
		) {
            SetMergedAreasAttributesOp setMergedAreasAttributesOp(borderCode, verbose);
			setMergedAreasAttributesOp._compute();
        }

        ///
        ///
        ///
        void SetMergedAreasAttributesOp::_init()
        {
            //--
            _logger = epg::log::EpgLoggerS::getInstance();
            _logger->log(epg::log::INFO, "[START] initialization: " + epg::tools::TimeTools::getTime());

            //--
            epg::Context *context = epg::ContextS::getInstance();

            //--
            epg::params::EpgParameters const& epgParams = context->getEpgParameters();
            std::string const areaTableName = epgParams.getValue(AREA_TABLE).toString();
            std::string const idName = epgParams.getValue(ID).toString();
            std::string const geomName = epgParams.getValue(GEOM).toString();
            std::string const countryName = epgParams.getValue(COUNTRY_CODE).toString();

			//--
			app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
			std::string const areaTableNameInitCleaned = themeParameters->getValue(AREA_TABLE_INIT_CLEANED).toString();
			std::string const listAttrSeparator = themeParameters->getValue(AM_LIST_ATTR_SEPARATOR).toString();
			std::string const listAttrJsonName = themeParameters->getValue(AM_LIST_ATTR_JSON).toString();
			std::string listAttrWName = themeParameters->getValue(AM_LIST_ATTR_W).toString();
			listAttrWName += listAttrSeparator + themeParameters->getValue(IS_STANDING_WATER_NAME).toString();

			//--
		    epg::tools::StringTools::Split(_borderCode, "#", _vCountry);

            //--
            _fsArea = context->getDataBaseManager().getFeatureStore(areaTableName, idName, geomName);

			//--
			_fsAreaInitCleaned = context->getDataBaseManager().getFeatureStore(areaTableNameInitCleaned, idName, geomName);

            //--
            _logger->log(epg::log::INFO, "[END] initialization: " + epg::tools::TimeTools::getTime());

			//--
			_attrMerger = new ome2::calcul::utils::AttributeMerger(
				listAttrWName,
				listAttrJsonName,
				countryName,
				listAttrSeparator,
				"#"
			);
        }

        ///
        ///
        ///
        void SetMergedAreasAttributesOp::_compute() const {

            // epg parameters
            epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
            std::string const idName = epgParams.getValue(ID).toString();
			//--
			app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
			std::string const wTagName = themeParameters->getValue(W_TAG_NAME).toString();
			std::string const isStandingWaterName = themeParameters->getValue(IS_STANDING_WATER_NAME).toString();

            ign::feature::FeatureFilter filterArea(countryCodeName+" = '#'");

            int numFeatures = ome2::feature::sql::NotDestroyedTools::NumFeatures(*_fsArea, filterArea);
            boost::progress_display display(numFeatures, std::cout, "[ setting merged areas attributes % complete ]\n");

            ign::feature::FeatureIteratorPtr itArea = ome2::feature::sql::NotDestroyedTools::GetFeatures(*_fsArea, filterArea);
            while (itArea->hasNext())
            {
                ++display;

                ign::feature::Feature fArea = itArea->next();
                std::string idOrigin = fArea.getId();
				//boucler sur les mp ?
				ign::geometry::MultiPolygon geomArea = fArea.getGeometry().asMultiPolygon();


				//DEBUG
				// if( geomArea.distance(ign::geometry::Point(4057801.86, 2934164.46)) < 1 ) {
				// 	bool test = true;
				// }

				ign::feature::Feature featCountry1, featCountry2;

				ign::feature::FeatureFilter filterArroundAreaFromCountry1(countryCodeName + " LIKE '%" + _vCountry[0] + "%'");
				filterArroundAreaFromCountry1.setExtent(geomArea.getEnvelope());

				double area1 = _getAreaMergedByCountry(geomArea, filterArroundAreaFromCountry1, featCountry1);

				ign::feature::FeatureFilter filterArroundAreaFromCountry2(countryCodeName + " LIKE '%"+ _vCountry[1] +"%'");
				filterArroundAreaFromCountry2.setExtent(geomArea.getEnvelope());

				double area2 = _getAreaMergedByCountry(geomArea, filterArroundAreaFromCountry2, featCountry2);

				if ( area1 < 0 && area2 < 0 ) {
					//pas d'attribut trouve
					fArea.setAttribute(wTagName, ign::data::String("modif_attr"));
					_fsArea->modifyFeature(fArea);
					continue;
				}
				else if ( area2 < 0.1*area1 ) {
					fArea = featCountry1;
				}
				else if ( area1 < 0.1*area2 ) {
					fArea = featCountry2;
				}
				else {
					ign::data::Variant isStanding1 = featCountry1.getAttribute(isStandingWaterName);
					ign::data::Variant isStanding2 = featCountry2.getAttribute(isStandingWaterName);

					fArea = _attrMerger->merge(featCountry1, featCountry2);

					if ( !isStanding1.isNull() && ! isStanding2.isNull() )
						fArea.setAttribute(isStandingWaterName, isStanding1);
				}
				fArea.setId(idOrigin);
				fArea.setGeometry(geomArea);
				fArea.setAttribute(wTagName, ign::data::String("modif_attr")); 
				fArea.setAttribute("xy_source", ign::data::String("ome2")); 
				fArea.setAttribute("z_source", ign::data::String("ome2"));
				_fsArea->modifyFeature(fArea);
            }
        }

		///
        ///
        ///
		double SetMergedAreasAttributesOp::_getAreaMergedByCountry(
			ign::geometry::MultiPolygon& geomAreaMerged,
			ign::feature::FeatureFilter& filterArroundAreaFromCountry,
			ign::feature::Feature& fMergedInit
		) const {

			std::map<double, ign::feature::Feature> mIntersectedArea;
			//recup fs table source -> table init sans step
			//filtre sur les feat de la table source
			ign::feature::FeatureIteratorPtr itAreaInit = ome2::feature::sql::NotDestroyedTools::GetFeatures(*_fsAreaInitCleaned, filterArroundAreaFromCountry);

			while (itAreaInit->hasNext())
			{

				ign::feature::Feature fAreaInit = itAreaInit->next();
				ign::geometry::MultiPolygon geomAreaInit = fAreaInit.getGeometry().asMultiPolygon();
				std::string idOriginInit = fAreaInit.getId();

				//si dist >0 continue sinon on stocke dans une map, id, feat
				if (geomAreaInit.distance(geomAreaMerged) > 0)
					continue;

				ign::geometry::GeometryPtr geomIntersectedArea(geomAreaInit.Intersection(geomAreaMerged));
				double areaIntersected = epg::tools::geometry::getArea(*geomIntersectedArea);

				if ( areaIntersected == 0 )
					continue;
				
				mIntersectedArea[areaIntersected] = fAreaInit;
			}

			if (mIntersectedArea.size() == 0) {
				fMergedInit.clear();
				return -1;
			}
			else {
				fMergedInit = mIntersectedArea.rbegin()->second;
				return mIntersectedArea.rbegin()->first;
			}
		}
    }
}