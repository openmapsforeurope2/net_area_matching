#include <app/utils/CopyTableUtils.h>

#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>
#include <epg/sql/DataBaseManager.h>
#include <epg/sql/tools/toSqlGeometryTypeName.h>

#include <boost/timer.hpp>

namespace app{
namespace utils{

	///
	///
	///
	bool CopyTableUtils::copyAreaTable( 
		std::string const& refTableName,
		std::string const& condition,
		bool askForConfirmation,
		bool typeIsMultipolygon,
		bool withData
	){
		epg::Context* context = epg::ContextS::getInstance();

		epg::log::ScopeLogger log("app::utils::copyAreaTable");
		std::cout << "[ copyAreaTable ]" << std::endl;
		boost::timer timer;

		std::string idName = context->getEpgParameters().getValue( ID ).toString();
		std::string geomName = context->getEpgParameters().getValue( GEOM ).toString();
		std::string tableName = context->getEpgParameters().getValue( AREA_TABLE ).toString();
		std::string countryName = context->getEpgParameters().getValue( COUNTRY_CODE ).toString();

		if( askForConfirmation && context->getDataBaseManager().tableExists( tableName ) )
		{
			std::string response = "";
			while( response != "O" && response != "o" && response != "N" && response != "n" )
			{
				std::cout << "Etes vous sur de vouloir ecraser la table " << tableName << " ? [O/N]" << std::endl;
				std::cin >> response;
			}

			if( response == "N" || response == "n" ) return false;
			IGN_ASSERT( response == "O" || response == "o" );
		}

		std::ostringstream ss;
		ss  << "DROP TABLE IF EXISTS " << tableName + ";"
            << "CREATE TABLE " << tableName << " AS ( SELECT * FROM " << refTableName ;

		if( !condition.empty() )
			ss  << " WHERE " << condition  ;

		ss << ")" ;

		if( !withData )
			ss << " WITH NO DATA ";

		ss  << ";"
			<< "ALTER TABLE " << tableName <<" ADD CONSTRAINT " << tableName << "_pkey PRIMARY KEY ("<< idName <<");"
			<< "ALTER TABLE " << tableName <<" ALTER COLUMN " << idName << " SET default gen_random_uuid();"
			<< "ALTER TABLE " << tableName <<" ALTER COLUMN " << idName << " SET NOT NULL;"
			<< "ALTER TABLE " << tableName <<" ADD CONSTRAINT enforce_geotype_geom CHECK (geometrytype("<< geomName <<") = " << ((typeIsMultipolygon)?"'MULTIPOLYGON'":"'POLYGON'") << "::text OR " << geomName <<" IS NULL);"
			<< "CREATE INDEX " << tableName +"_country_idx ON " << tableName << " USING btree ("<< countryName <<" ASC NULLS LAST);"
            << "CREATE INDEX " << tableName +"_objectid_idx ON " << tableName << " USING btree ("<< idName <<" ASC NULLS LAST);"
            << "CREATE INDEX " << tableName +"_w_step_idx ON " << tableName << " USING btree (w_step ASC NULLS LAST);"
			<< "CREATE INDEX " << tableName +"_geom_idx ON " << tableName << " USING GIST ("<< geomName <<");";


		context->getDataBaseManager().getConnection()->update( ss.str() ); 


		log.log( epg::log::INFO, "Execution time :" + ign::data::Double( timer.elapsed() ).toString() ) ;

		return true;
	}
}
}