/*******************************************************
源程序名称: fix_tag.h
软件著作权: 恒生电子股份有限公司
系统名称  : 06版本期货系统
模块名称  : 恒生期货FIX协议接口
功能说明  : FIX-TAG值定义:
            1)标准(FIX4.0-4.4)
			2)自定义
作    者  : xdx
开发日期  : 20091201
备    注  :  

修改人员  ：
修改日期  ：
修改说明  ：20091201 创建
*********************************************************/
#ifndef _FIX_TAG_H_
   #define _FIX_TAG_H_

//引擎的标准字段
#include "FieldNumbers.h"

//定义额外的扩展字段
namespace FIELD
{
  enum Field_Append
  {
	 CustomerOrFirm = 204,
     MinPriceIncrement = 969,
	 MDFeedType = 1022,
	 MDPriceLevel = 1023,
	 ManualOrderIndicator = 1028,
	 AggressorIndicator = 1057,
	 NoMdFeedTypes = 1141,
	 EventTime = 1145,
	 ApplID = 1180,
	 MDSecurityTradingStatus = 1682,
	 Offset = 5001,
	 MatchEventIndicator = 5799,
	 TickRule = 6350,
	 Asset = 6937,
	 SelfMatchPreventionID = 7928,
	 CtiCode = 9702,
	 CorrelationClOrdID = 9717,
	 OFMOverride = 9768,
	 DisplayFactor = 9787,
	 QuoteType = 9943,
	 MainFraction = 37702
  };
}

#endif

