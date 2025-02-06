<?xml version="1.0" encoding="US-ASCII"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:output method="text" encoding="kodirowka:cp866 ili US-ASCII" omit-xml-declaration="yes"/>
<xsl:template match="/">
<xsl:message>
<xsl:text>AP2R:wyzwano preobrazowanie: </xsl:text>
<xsl:text>&lt;</xsl:text>
<xsl:value-of select="local-name(/*)"/>
<xsl:text> </xsl:text>
<xsl:for-each select="/*/@*">
<xsl:value-of select="local-name(.)"/><xsl:text>="</xsl:text><xsl:value-of select="."/><xsl:text>"</xsl:text>
<xsl:if test="position()!=last()"><xsl:text> </xsl:text></xsl:if>
</xsl:for-each>
<xsl:text>&gt;</xsl:text>
</xsl:message>
<xsl:for-each select="/*">
<xsl:choose>
<xsl:when test="local-name(.)='A'">
<xsl:apply-templates mode="ROOT" select="."/>
</xsl:when>
<xsl:otherwise>
<xsl:message terminate="yes">
<xsl:text>AP2R:o{ibka formirowaniq otweta dlq preobrazowaniq: wmesto uzla /A obnaruven uzel /</xsl:text>
<xsl:value-of select="local-name(.)"/>
</xsl:message>
</xsl:otherwise>
</xsl:choose>
</xsl:for-each>
</xsl:template>
<xsl:template name="YMD2DMY">
<xsl:param name="YMD"/>
<xsl:value-of select="substring($YMD, 9, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring($YMD, 6, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring($YMD, 1, 4)"/>
</xsl:template>
<xsl:template match="ZG">
<xsl:call-template name="YMD2DMY">
   <xsl:with-param name="YMD" select="@DZ"/>
</xsl:call-template>
<xsl:text> </xsl:text>
<xsl:value-of select="substring(@TZ,1,8)"/>
<xsl:text> </xsl:text>
<xsl:value-of select="IZK"/>
<xsl:text> </xsl:text>
<xsl:value-of select="TM/@VC"/>
<xsl:value-of select="TM/NPP"/>
<xsl:value-of select="TM/SDOR"/>
<xsl:value-of select="TM/PNTM"/>
</xsl:template>
<xsl:template name="ZAYAVKA">
<xsl:param name="MODE"/>
<xsl:text>dlq proezda nedejstwitelen</xsl:text>/CRLF
<xsl:choose>
<xsl:when test="OTKAZ">
<xsl:text>otkaz ot zaqwki</xsl:text>/CRLF
</xsl:when>
<xsl:otherwise>
<xsl:text>prodlenie sroka zaqwki</xsl:text>/CRLF
</xsl:otherwise>
</xsl:choose>
<xsl:choose>
<xsl:when test="INFOZVK/NBZ">
<xsl:text>bronx-zakaz </xsl:text>
<xsl:value-of select="INFOZVK/NBZ"/>
<xsl:text> </xsl:text>
</xsl:when>
</xsl:choose>
<xsl:choose>
<xsl:when test="INFOZVK/ZKOD">
<xsl:value-of select="INFOZVK/ZKOD"/>
<xsl:text> </xsl:text>
</xsl:when>
</xsl:choose>
<xsl:choose>
<xsl:when test="INFOZVK/ANYPP">
<xsl:text>na l`bom pp</xsl:text>
</xsl:when>
<xsl:otherwise>
<xsl:text>dlq pp </xsl:text>
<xsl:value-of select="INFOZVK/NPP"/>
</xsl:otherwise>
</xsl:choose>
/CRLF
<xsl:if test="INFOZVK/DY">
<xsl:text>data qwki </xsl:text>
<xsl:value-of select="substring(INFOZVK/DY, 9, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring(INFOZVK/DY, 6, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring(INFOZVK/DY, 1, 4)"/>
</xsl:if>
<xsl:if test="INFOZVK/TY">
<xsl:text> wremq qwki </xsl:text>
<xsl:value-of select="substring(INFOZVK/TY, 1, 2)"/>
<xsl:text>:</xsl:text>
<xsl:value-of select="substring(INFOZVK/TY, 4, 2)"/>
/CRLF
</xsl:if>
<xsl:if test="$MODE='P'">/NAK/ESC{/ESC!8003030/ESC#/POS02/AVT/CPI20</xsl:if>
<xsl:for-each select="TRAIN">
<xsl:for-each select="PP">
/CRLF
<xsl:choose>
<xsl:when test="GRP">
<xsl:text>dlq gruppy </xsl:text>
</xsl:when>
<xsl:otherwise>
<xsl:text>dlq indiwidualxnyh passavirow </xsl:text>
</xsl:otherwise>
</xsl:choose>
<xsl:if test="SGRP = '001'">
<xsl:text>detej </xsl:text>
</xsl:if>
<xsl:if test="SGRP = '002'">
<xsl:text>{kolxnikow </xsl:text>
</xsl:if>
<xsl:if test="SGRP = '003'">
<xsl:text>woennosluva}ih </xsl:text>
</xsl:if>
<xsl:if test="SGRP = '004'">
<xsl:text>prizywnikow </xsl:text>
</xsl:if>
<xsl:if test="SGRP = '005' or SGRP = '007'">
<xsl:text>passavirow </xsl:text>
</xsl:if>
<xsl:if test="MK">
<xsl:text>mest </xsl:text>
<xsl:value-of select="MK"/>
</xsl:if>
<xsl:if test="NBR">
<xsl:text> iz broni </xsl:text>
<xsl:value-of select="NBR"/>
</xsl:if>
<xsl:if test="CLIENT">
<xsl:text> dlq </xsl:text>
<xsl:value-of select="CLIENT"/>
</xsl:if>
/CRLF
<xsl:text>poezd </xsl:text>
<xsl:value-of select="N1"/>
<xsl:text> otprawleniem </xsl:text>
<xsl:value-of select="substring(DEP/D3, 9, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring(DEP/D3, 6, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring(DEP/D3, 1, 4)"/>
<xsl:text> w </xsl:text>
<xsl:value-of select="substring(DEP/T1, 1, 2)"/>
<xsl:text>:</xsl:text>
<xsl:value-of select="substring(DEP/T1, 4, 2)"/>
<xsl:text> </xsl:text>
/CRLF
<xsl:value-of select="parent::*/parent::*/NP/C1"/>
<xsl:text> (</xsl:text>
<xsl:value-of select="parent::*/parent::*/NP/CC1"/>
<xsl:text>) - </xsl:text>
<xsl:value-of select="parent::*/parent::*/NP/C2"/>
<xsl:text> (</xsl:text>
<xsl:value-of select="parent::*/parent::*/NP/CC2"/>
<xsl:text>)</xsl:text>
/CRLF
<xsl:if test="$MODE='P'">/NAK/ESC{/ESC!8003030/ESC#/POS02/AVT/CPI20</xsl:if>
<xsl:for-each select="CAR">
<xsl:choose>
<xsl:when test="string-length(VN) = 2">
<xsl:text>wagon  </xsl:text>
<xsl:value-of select="VN"/>
</xsl:when>
<xsl:when test="string-length(VN) = 3">
<xsl:text>wagon </xsl:text>
<xsl:value-of select="VN"/>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
<xsl:if test="string-length(KV) &gt; 0">
<xsl:text> </xsl:text>
<xsl:value-of select="KV"/>
</xsl:if>
<xsl:if test="string-length(DB) &gt; 0">
<xsl:text>/</xsl:text>
<xsl:value-of select="DB"/>
<xsl:text>*</xsl:text>
</xsl:if>
<xsl:if test="CARL">
<xsl:text> mest </xsl:text>
<xsl:if test="string-length(NN/MK1) &gt; 0">
<xsl:value-of select="NN/MK1"/>
</xsl:if>
<xsl:text>/</xsl:text>
/POS23/AHT
<xsl:for-each select="NN/N1">
<xsl:value-of select="."/>
<xsl:if test="position() != last()">
<xsl:text>;</xsl:text>
</xsl:if>
<xsl:if test="position() mod 9 = 0">
/CRLF
<xsl:if test="position() mod 486 = 0">
<xsl:text>/NAK/ESC{/ESC!8003030/ESC#/POS02/AVT/CPI20</xsl:text>
</xsl:if>
/POS23/AHT
</xsl:if>
</xsl:for-each>
/CRLF
</xsl:if>
<xsl:if test="$MODE='P'">/NAK/ESC{/ESC!8003030/ESC#/POS02/AVT/CPI20</xsl:if>
</xsl:for-each>
</xsl:for-each>
</xsl:for-each>
/CRLF<xsl:text>itogo s passavira: </xsl:text>
<xsl:value-of select="PAY_PASS/TF"/>
<xsl:text> rub (w t.~. nds: </xsl:text>
<xsl:value-of select="PAY_PASS/STV"/>
<xsl:text>% </xsl:text>
<xsl:value-of select="PAY_PASS/NDS"/>
<xsl:text>)</xsl:text>
/CRLF
<xsl:text>itogo s wedomstwa: </xsl:text>
<xsl:value-of select="PAY_VED/TF"/>
<xsl:text> rub (w t.~. nds: </xsl:text>
<xsl:value-of select="PAY_VED/STV"/>
<xsl:text>% </xsl:text>
<xsl:value-of select="PAY_VED/NDS"/>
<xsl:text>)</xsl:text>
/CRLF
<xsl:choose>
<xsl:when test="NO_ELS">
<xsl:text>els: </xsl:text>
<xsl:value-of select="ELS"/>
<xsl:text> ne najden w sprawo~nike</xsl:text>
</xsl:when>
<xsl:otherwise>
<xsl:text>els: </xsl:text>
<xsl:value-of select="ELS"/>
<xsl:text> wladelec: </xsl:text>
<xsl:value-of select="ELS_OW"/>
</xsl:otherwise>
</xsl:choose>
/CRLF
/CRLF
<xsl:apply-templates select="ZG"/>
</xsl:template>
<xsl:template mode="T8020" match="RESP">
<xsl:call-template name="ZAYAVKA">
<xsl:with-param name="MODE" select="'D'"/>
</xsl:call-template>
</xsl:template>
<xsl:template mode="P" match="RESP">
<xsl:text disable-output-escaping="yes">/ESC{/ESC!8003030</xsl:text>
<xsl:text disable-output-escaping="yes">/ESC#/POS02/AVT/CPI20</xsl:text>
<xsl:call-template name="ZAYAVKA">
<xsl:with-param name="MODE" select="'P'"/>
</xsl:call-template>
<xsl:text disable-output-escaping="yes">/FF</xsl:text>
</xsl:template>
<xsl:template mode="ERROR" match="RESP"><xsl:text disable-output-escaping="yes">informaciq ob o{ibke: </xsl:text><xsl:value-of select="ERROR/INFO"/></xsl:template>
<xsl:template mode="BAD_PARS" match="A">
<xsl:message terminate="yes">
<xsl:text disable-output-escaping="yes">AP2R:neprawilxno zadany atributy: </xsl:text>
<xsl:text>&lt;</xsl:text>
<xsl:value-of select="local-name(.)"/>
<xsl:text> </xsl:text>
<xsl:for-each select="@*">
<xsl:value-of select="local-name(.)"/><xsl:text>="</xsl:text><xsl:value-of select="."/><xsl:text>"</xsl:text>
<xsl:if test="position()!=last()"><xsl:text> </xsl:text></xsl:if>
</xsl:for-each>
<xsl:text>&gt;</xsl:text>
</xsl:message>
</xsl:template>
<xsl:template mode="BAD_TYPE" match="RESP">
<xsl:message terminate="yes">
<xsl:text disable-output-escaping="yes">AP2R:neovidannyj tip otweta: "</xsl:text>
<xsl:value-of select="@TYPE"/><xsl:text disable-output-escaping="yes">"</xsl:text>
</xsl:message>
</xsl:template>
<xsl:template mode="ROOT" match="A">
<xsl:choose>
<xsl:when test="count(RESP)=0">
<xsl:message terminate="yes">
<xsl:text disable-output-escaping="yes">AP2R:naru{ena struktura otweta: net uzla RESP</xsl:text>
</xsl:message>
</xsl:when>
<xsl:when test="count(RESP)!=1">
<xsl:message terminate="yes">
<xsl:text disable-output-escaping="yes">AP2R:naru{ena struktura otweta: koli~estwo uzlow RESP bolee 1: </xsl:text><xsl:value-of select="count(RESP)"/>
</xsl:message>
</xsl:when>
<xsl:when test="count(RESP/@TYPE)=0">
<xsl:message terminate="yes">
<xsl:text disable-output-escaping="yes">AP2R:naru{ena struktura otweta: u uzla RESP net atributa TYPE</xsl:text>
</xsl:message>
</xsl:when>
<xsl:when test="RESP/@TYPE ='ERROR'">
<xsl:apply-templates mode="ERROR" select="RESP"/>
</xsl:when>
<xsl:when test="RESP/@TYPE ='ZAYAVKA'">
<xsl:choose>
<xsl:when test="@W='80' and @H='20' and substring(@T,1,1)='S'">
<xsl:apply-templates mode="T8020" select="RESP"/>
</xsl:when>
<xsl:when test="substring(@T,1,1)='P'">
<xsl:apply-templates mode="P" select="RESP"/>
</xsl:when>
<xsl:otherwise>
<xsl:apply-templates mode="BAD_PARS" select="."/>
</xsl:otherwise>
</xsl:choose>
</xsl:when>
<xsl:otherwise>
<xsl:apply-templates mode="BAD_TYPE" select="RESP"/>
</xsl:otherwise>
</xsl:choose>
</xsl:template>
</xsl:stylesheet>
