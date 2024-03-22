// APP
#include <app/calcul/CfCleanerOp.h>
#include <app/params/ThemeParameters.h>

// EPG
#include <epg/Context.h>
#include <epg/params/EpgParameters.h>
#include <epg/sql/DataBaseManager.h>
#include <epg/tools/StringTools.h>
#include <epg/tools/TimeTools.h>


namespace app
{
    namespace calcul
    {
        ///
        ///
        ///
        CfCleanerOp::CfCleanerOp(
            bool verbose
        ) : 
            _verbose(verbose)
        {
            _init();
        }

        ///
        ///
        ///
        CfCleanerOp::~CfCleanerOp()
        {
            _shapeLogger->closeShape("ccf_deleted_cp");
            _shapeLogger->closeShape("ccf_deleted_cl");
        }

        ///
        ///
        ///
        void CfCleanerOp::compute(
			bool verbose
		) {
            CfCleanerOp CfCleanerOp(verbose);
            CfCleanerOp._compute();
        }


        ///
        ///
        ///
        void CfCleanerOp::_init()
        {
            //--
            _logger = epg::log::EpgLoggerS::getInstance();
            _logger->log(epg::log::INFO, "[START] initialization: " + epg::tools::TimeTools::getTime());

            //--
            _shapeLogger = epg::log::ShapeLoggerS::getInstance();
            _shapeLogger->addShape("ccf_deleted_cp", epg::log::ShapeLogger::POINT);
            _shapeLogger->addShape("ccf_deleted_cl", epg::log::ShapeLogger::LINESTRING);
            
            //--
            epg::Context *context = epg::ContextS::getInstance();

            // epg parameters
            epg::params::EpgParameters const& epgParams = context->getEpgParameters();
            std::string const areaTableName = epgParams.getValue(AREA_TABLE).toString();
            std::string const idName = epgParams.getValue(ID).toString();
            std::string const geomName = epgParams.getValue(GEOM).toString();

            // app parameters
            params::ThemeParameters * themeParameters = params::ThemeParametersS::getInstance();
            std::string cpTableName = themeParameters->getValue(CUTP_TABLE).toString();
            if (cpTableName == "") {
                std::string const cpTableSuffix = themeParameters->getValue(CUTP_TABLE_SUFFIX).toString();
                cpTableName = areaTableName + cpTableSuffix;
            }
            std::string clTableName = themeParameters->getValue(CUTL_TABLE).toString();
            if (clTableName == "") {
                std::string const clTableSuffix = themeParameters->getValue(CUTL_TABLE_SUFFIX).toString();
                clTableName = areaTableName + clTableSuffix;
            }

            //--
            _fsArea = context->getDataBaseManager().getFeatureStore(areaTableName, idName, geomName);

            //--
            _fsCp = context->getDataBaseManager().getFeatureStore(cpTableName, idName, geomName);

            //--
            _fsCl = context->getDataBaseManager().getFeatureStore(clTableName, idName, geomName);

            //--
            _logger->log(epg::log::INFO, "[END] initialization: " + epg::tools::TimeTools::getTime());
        };

        ///
        ///
        ///
        void CfCleanerOp::_compute() const {
            // epg parameters
            epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
            std::string const linkedFeatIdName = epgParams.getValue(LINKED_FEATURE_ID).toString();

            //--
            std::set<std::string> sCp2Delete;

            ign::feature::FeatureFilter filterCp;
            ign::feature::FeatureIteratorPtr itCp = _fsCp->getFeatures(filterCp);
            while (itCp->hasNext())
            {
                ign::feature::Feature const& fCp = itCp->next();
                std::string cpId = fCp.getId();
                std::string linkedFeatId = fCp.getAttribute(linkedFeatIdName).toString();

                ign::feature::Feature fArea;
                _fsArea->getFeatureById(linkedFeatId, fArea);
                
                if( fArea.getId() != "") {
                    sCp2Delete.insert(cpId);

                    _shapeLogger->writeFeature("ccf_deleted_cp", fCp);
                }
            }

            for ( std::set<std::string>::const_iterator sit = sCp2Delete.begin() ; sit != sCp2Delete.end() ; ++sit) {
                _fsCp->deleteFeature(*sit);
            }


            //--
            std::set<std::string> sCl2Delete;

            ign::feature::FeatureFilter filterCl;
            ign::feature::FeatureIteratorPtr itCl = _fsCl->getFeatures(filterCl);
            while (itCl->hasNext())
            {
                ign::feature::Feature const& fCl = itCl->next();
                std::string clId = fCl.getId();
                std::string linkedFeatId = fCl.getAttribute(linkedFeatIdName).toString();

                if (clId == "16702069-36f2-41c3-b0f1-bcf4a8e91a78") {
                    bool test = true;
                }

                std::vector<std::string> vIds;
                epg::tools::StringTools::Split(linkedFeatId, "#", vIds);

                bool bDelete = true;
                for (size_t i = 0 ; i < vIds.size() ; ++i) {
                    ign::feature::Feature fArea;
                    _fsArea->getFeatureById(vIds[i], fArea);
                    if(fArea.getId() != "") {
                        bDelete = false;
                        break;
                    }
                }

                if( bDelete ) {
                    sCl2Delete.insert(clId);

                    _shapeLogger->writeFeature("ccf_deleted_cl", fCl);
                }
            }

            for ( std::set<std::string>::const_iterator sit = sCl2Delete.begin() ; sit != sCl2Delete.end() ; ++sit) {
                _fsCl->deleteFeature(*sit);
            }
        };
    }
}
