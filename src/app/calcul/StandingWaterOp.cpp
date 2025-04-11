// APP
#include <app/calcul/StandingWaterOp.h>
#include <app/params/ThemeParameters.h>

// BOOST
#include <boost/progress.hpp>

// EPG
#include <epg/Context.h>
#include <epg/tools/TimeTools.h>
#include <epg/tools/StringTools.h>

#include <ome2/feature/sql/NotDestroyedTools.h>


namespace app
{
    namespace calcul
    {
		///
		///
		///
		StandingWaterOp::StandingWaterOp(
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
		StandingWaterOp::~StandingWaterOp()
		{
		}



		///
		///
		///
		void StandingWaterOp::_init()
		{
			//--
			_logger = epg::log::EpgLoggerS::getInstance();
			_logger->log(epg::log::INFO, "[START] initialization: " + epg::tools::TimeTools::getTime());

			//--
			epg::Context *context = epg::ContextS::getInstance();

			// epg parameters
			epg::params::EpgParameters const& epgParams = context->getEpgParameters();
			std::string const areaTableName = epgParams.getValue(AREA_TABLE).toString();
			std::string const idName = epgParams.getValue(ID).toString();
			std::string const geomName = epgParams.getValue(GEOM).toString();
					
			// app parameters
			params::ThemeParameters *themeParameters = params::ThemeParametersS::getInstance();

			//--
			epg::tools::StringTools::Split(_borderCode, "#", _vCountriesCodeName);

			//--
			_fsArea = context->getDataBaseManager().getFeatureStore(areaTableName, idName, geomName);

			//--
			std::string areaStandingWaterTableName = themeParameters->getValue(AREA_TABLE_INIT_STANDING_WATER).toString();
			_fsAreaStandingWater = context->getDataBaseManager().getFeatureStore(areaStandingWaterTableName, idName, geomName);

			//--
			_attrValueStandingWater = "standing_water";

			//--
			_logger->log(epg::log::INFO, "[END] initialization: " + epg::tools::TimeTools::getTime());
		};


		///
		///
		///
		void StandingWaterOp::AddStandingWater(
			std::string borderCode,
			bool verbose
		) {
			StandingWaterOp standingWaterOp(borderCode, verbose);
			standingWaterOp._addStandingWater();
			
		}

		///
		///
		///
		void StandingWaterOp::_addStandingWater() const
		{
			//--
			epg::Context *context = epg::ContextS::getInstance();

			// epg parameters
			epg::params::EpgParameters const& epgParams = context->getEpgParameters();
			std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
			std::string const idName = epgParams.getValue(ID).toString();
			std::string const geomName = epgParams.getValue(GEOM).toString();

			// app parameters
			params::ThemeParameters *themeParameters = params::ThemeParametersS::getInstance();
			std::string attrIsStandingWaterName = themeParameters->getValue(IS_STANDING_WATER_NAME).toString();

			//--
			std::ostringstream ss;
			ss << "ALTER TABLE " << _fsArea->getTableName() << " ADD COLUMN " << attrIsStandingWaterName << " character varying(255);";
			context->getDataBaseManager().getConnection()->update(ss.str());
			
			//--
			context->getDataBaseManager().refreshFeatureStore(_fsArea->getTableName(),idName, geomName);

			//--
			ign::feature::FeatureFilter filterCountries("(" + countryCodeName + " = '" + _vCountriesCodeName[0] + "' or " + countryCodeName + " = '" + _vCountriesCodeName[1] + "')");

			int numFeaturesStandingWater = ome2::feature::sql::numFeatures(*_fsAreaStandingWater, filterCountries);
			boost::progress_display display(numFeaturesStandingWater, std::cout, "[ IMPORT STANDING WATER ]\n");

			std::set<std::string> sIdStandingArea2delete;

			ign::feature::FeatureIteratorPtr itStandingArea = ome2::feature::sql::getFeatures(_fsAreaStandingWater,filterCountries);
			while (itStandingArea->hasNext()) {
				++display;
				ign::feature::Feature fStandingArea = itStandingArea->next();

				std::string countryCodeStandingArea = fStandingArea.getAttribute(countryCodeName).toString();

				fStandingArea.setAttribute(attrIsStandingWaterName, ign::data::String(_attrValueStandingWater));
				sIdStandingArea2delete.insert(fStandingArea.getId());
				_fsArea->createFeature(fStandingArea);
			}

			//--
			for (std::set<std::string>::iterator sit = sIdStandingArea2delete.begin(); sit != sIdStandingArea2delete.end(); ++sit)
				_fsAreaStandingWater->deleteFeature(*sit);
		}

		///
		///
		///
		void StandingWaterOp::SortingStandingWater(
			std::string borderCode,
			bool verbose
		) {
			StandingWaterOp standingWaterOp(borderCode, verbose);
			standingWaterOp._sortingStandingWater();
		}

		///
		///
		///
		void StandingWaterOp::_sortingStandingWater() const
		{

			//--
			epg::Context *context = epg::ContextS::getInstance();

			// app parameters
			params::ThemeParameters *themeParameters = params::ThemeParametersS::getInstance();
			std::string attrIsStandingWaterName = themeParameters->getValue(IS_STANDING_WATER_NAME).toString();

			//--
			ign::feature::FeatureFilter filterStandingArea(attrIsStandingWaterName + " = '" + _attrValueStandingWater + "'");

			int numFeaturesStandingWater = ome2::feature::sql::numFeatures(*_fsArea, filterStandingArea);
			boost::progress_display display(numFeaturesStandingWater, std::cout, "[ EXPORT STANDING WATER ]\n");

			std::set<std::string> sIdStandingArea2delete;

			ign::feature::FeatureIteratorPtr itStandingArea = ome2::feature::sql::getFeatures(_fsArea,filterStandingArea);
			while (itStandingArea->hasNext()) {
				++display;
				ign::feature::Feature fStandingArea = itStandingArea->next();

				sIdStandingArea2delete.insert(fStandingArea.getId());
				_fsAreaStandingWater->createFeature(fStandingArea);
			}

			//--
			for (std::set<std::string>::iterator sit = sIdStandingArea2delete.begin(); sit != sIdStandingArea2delete.end(); ++sit)
				_fsArea->deleteFeature(*sit);

			//--
			std::ostringstream ss;
			ss << "ALTER TABLE " << _fsArea->getTableName() << " DROP COLUMN " << attrIsStandingWaterName << ";";
			context->getDataBaseManager().getConnection()->update(ss.str());
		}
	}
}