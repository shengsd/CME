/*******************************************************
Դ��������: fix_tag.h
�������Ȩ: �������ӹɷ����޹�˾
ϵͳ����  : 06�汾�ڻ�ϵͳ
ģ������  : �����ڻ�FIXЭ��ӿ�
����˵��  : FIX-TAGֵ����:
            1)��׼(FIX4.0-4.4)
			2)�Զ���
��    ��  : xdx
��������  : 20091201
��    ע  :  

�޸���Ա  ��
�޸�����  ��
�޸�˵��  ��20091201 ����
*********************************************************/
#ifndef _FIX_TAG_H_
   #define _FIX_TAG_H_

//����ı�׼�ֶ�
#include "FieldNumbers.h"

//����������չ�ֶ�
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

