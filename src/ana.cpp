#include "ThostFtdcUserApiStruct.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/regex.hpp>    

#include <deque>
#include <map>
#include <set>
#include <fstream>
#include <iostream>
#include <string>

using namespace std;

namespace fs = boost::filesystem;
namespace pt = boost::posix_time;
namespace dt = boost::date_time;

// 同类型合约下的所有合约名称
typedef deque<string> InstrumentIdSet;

// 合约的所有市场数据
typedef deque<pair<pt::ptime, CThostFtdcDepthMarketDataField> > DepthMarketDataSet;
typedef map<string, DepthMarketDataSet> InstrumentMarketDataSet;

static const string DATA_PATH = "./data";

static const std::locale loc =
    std::locale(std::locale::classic(),
		new pt::time_input_facet("%Y-%m-%d %H:%M:%S"));

pt::ptime ana_convert_date_time_from_string(string& date_time)
{

    istringstream iss(date_time);
    iss.imbue(loc);
    boost::posix_time::ptime t;
    iss >> t;
    return t;
}

void ana_instrument_calc(string instrument1, string instrument2)
{
}

/*
  统计合约的平均值，方差。
 */
void ana_cacl(InstrumentMarketDataSet& instrument_market_data_set)
{
    for (InstrumentMarketDataSet::iterator i = instrument_market_data_set.begin();
	 i != instrument_market_data_set.end();
	 i++)
    {
	for (InstrumentMarketDataSet::iterator j = instrument_market_data_set.begin();
	     j != instrument_market_data_set.end();
	     j++)
	{
	    if ((*i).first == (*j).first) continue;
	    cout << (*i).first << "," << (*j).first << endl;
	    
	}
    }
}

/*
  一个合约，每天会生成一个csv。把csv中每条记录转为DepthMarketData结构
 */
void ana_instruments_get_market_data_from_csv(std::string instrument_date_file, DepthMarketDataSet& depth_market_data_set)
{
    ifstream ifs(instrument_date_file.c_str());
    string line;
    while (getline(ifs, line))
    {
	stringstream line_stream(line);
	string cell;
	deque<string> cells;
	CThostFtdcDepthMarketDataField depth_market_data;
	memset((void*)&depth_market_data, 0, sizeof(CThostFtdcDepthMarketDataField));

	// Datetime
	getline(line_stream, cell, ',');
	pt::ptime dt = ana_convert_date_time_from_string(cell);

	// 交易日
	// TThostFtdcDateType	TradingDay
	getline(line_stream, cell, ',');
	strcpy(depth_market_data.TradingDay, cell.c_str());

	///合约代码
	// TThostFtdcInstrumentIDType	InstrumentID
	getline(line_stream, cell, ',');
	strcpy(depth_market_data.InstrumentID, cell.c_str());

	///交易所代码
	// TThostFtdcExchangeIDType	ExchangeID;
	getline(line_stream, cell, ',');

	///合约在交易所的代码
	// TThostFtdcExchangeInstIDType	ExchangeInstID;
	getline(line_stream, cell, ',');

	///最新价
	// TThostFtdcPriceType	LastPrice;
	getline(line_stream, cell, ',');
	char* p;
	depth_market_data.LastPrice = strtod(cell.c_str(), &p);

	///上次结算价
	// TThostFtdcPriceType	PreSettlementPrice;
	getline(line_stream, cell, ',');

	///昨收盘
	// TThostFtdcPriceType	PreClosePrice;
	getline(line_stream, cell, ',');
	
	///昨持仓量
	// TThostFtdcLargeVolumeType	PreOpenInterest;
	getline(line_stream, cell, ',');
	
	///今开盘
	// TThostFtdcPriceType	OpenPrice;
	getline(line_stream, cell, ',');
	
	///最高价
	// TThostFtdcPriceType	HighestPrice;
	getline(line_stream, cell, ',');
	
	///最低价
	// TThostFtdcPriceType	LowestPrice;
	getline(line_stream, cell, ',');

	///数量
	// TThostFtdcVolumeType	Volume;
	getline(line_stream, cell, ',');	
	depth_market_data.Volume = strtod(cell.c_str(), &p);
	
	///成交金额
	// TThostFtdcMoneyType	Turnover;
	getline(line_stream, cell, ',');	
	depth_market_data.Turnover = strtod(cell.c_str(), &p);

	depth_market_data_set.push_back(make_pair(dt, depth_market_data));
    }
}

/*
  读取一个合约的所有数据。
 */
void ana_instruments_get_market_data(string instrument_id, InstrumentMarketDataSet& instrument_market_data_set)
{
    fs::path full_path(fs::system_complete(fs::path(DATA_PATH)));
    if (!fs::exists(full_path))
    {
	cerr << "Can not find data path..." << endl;
	exit(-1);
    }
    
    DepthMarketDataSet depth_market_data_set;
    string wildcard_filter = instrument_id + ".*";
    boost::regex filter_regex(wildcard_filter);
    fs::directory_iterator end_itr;
    for (fs::directory_iterator itr(DATA_PATH); itr!=end_itr; itr++)
    {
	if (!boost::regex_match(itr->path().filename().string(), filter_regex))
	    continue;
//	cout << itr->path().filename().string() << endl;
	instrument_market_data_set[instrument_id] = depth_market_data_set;
	ana_instruments_get_market_data_from_csv(itr->path().string(), depth_market_data_set);
    }

    cerr << instrument_id << " has " << depth_market_data_set.size() << " record(s)..." << endl;
}

void ana_instruments_calc_by_type(string instrument_type, InstrumentMarketDataSet& instrument_market_data_set)
{
    ifstream ifs("./etc/instruments");
    string instrument_id;
    InstrumentIdSet instrument_id_queue;
    while (getline(ifs, instrument_id))
    {
	if (instrument_id.substr(0, instrument_type.size()) == instrument_type)
	{
	    instrument_id_queue.push_back(instrument_id);
	    ana_instruments_get_market_data(instrument_id, instrument_market_data_set);
	}
    }
}

int main()
{
    InstrumentMarketDataSet instrument_market_data_set;
    ana_instruments_calc_by_type("cu", instrument_market_data_set);

    ana_cacl(instrument_market_data_set);
}
