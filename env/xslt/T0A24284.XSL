<?xml version="1.0" encoding="US-ASCII"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:output method="text" encoding="US-ASCII" omit-xml-declaration="yes"/>
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
<xsl:template name="processDLO">
<xsl:param name="DLV"/>
<xsl:text>msk</xsl:text>
<xsl:choose>
<xsl:when test="$DLV">
<xsl:choose>
<xsl:when test="substring($DLV, 1, 1) = '-'">
<xsl:value-of select="$DLV"/>
</xsl:when>
<xsl:otherwise>
<xsl:choose>
<xsl:when test="$DLV = '0' or $DLV = '00'">
</xsl:when>
<xsl:when test="string-length($DLV) = 2 and substring($DLV, 1, 1) = '0'">
<xsl:text>+</xsl:text>
<xsl:value-of select="substring($DLV, 2)"/>
</xsl:when>
<xsl:otherwise>
<xsl:text>+</xsl:text>
<xsl:value-of select="$DLV"/>
</xsl:otherwise>
</xsl:choose>
</xsl:otherwise>
</xsl:choose>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
</xsl:template>
<xsl:template name="processDLP">
<xsl:param name="DLV1"/>
<xsl:text>msk</xsl:text>
<xsl:choose>
<xsl:when test="$DLV1">
<xsl:choose>
<xsl:when test="substring($DLV1, 1, 1) = '-'">
<xsl:value-of select="$DLV1"/>
</xsl:when>
<xsl:otherwise>
<xsl:choose>
<xsl:when test="$DLV1 = '0' or $DLV1 = '00'">
</xsl:when>
<xsl:when test="string-length($DLV1) = 2 and substring($DLV1, 1, 1) = '0'">
<xsl:text>+</xsl:text>
<xsl:value-of select="substring($DLV1, 2)"/>
</xsl:when>
<xsl:otherwise>
<xsl:text>+</xsl:text>
<xsl:value-of select="$DLV1"/>
</xsl:otherwise>
</xsl:choose>
</xsl:otherwise>
</xsl:choose>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
</xsl:template>
<xsl:template name="NUM2POS">
<xsl:param name="NUM"/>
<xsl:variable name="POS" select="concat('00',$NUM)"/>
<xsl:value-of select="substring($POS,string-length($POS)-1)"/>
</xsl:template>
<xsl:template mode="T8020" match="RESP">
<xsl:call-template name="ZAGOLOVOK1"/>
<xsl:choose>
<xsl:when test="TRAIN">
<xsl:apply-templates select="TRAIN"/>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
</xsl:template>
<xsl:template match="PP">
/CRLF
/POS19/AHT<xsl:value-of select="N1"/>
<xsl:text>, </xsl:text>
<xsl:value-of select="Z1/C1"/>
<xsl:text>-</xsl:text>
<xsl:value-of select="Z1/C2"/>
</xsl:template>
<xsl:template match="TRAIN">
<xsl:apply-templates select="PP"/>
<xsl:choose>
<xsl:when test="CARRIER/VB">
<xsl:text>, </xsl:text>
<xsl:value-of select="CARRIER/VB"/>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
<xsl:choose>
<xsl:when test="FIRM">
<xsl:text>, firmennyj</xsl:text>
<xsl:value-of select="FIRM"/>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
/CRLF
/POS00/AHT<xsl:text>otprawlenie</xsl:text>
/POS13/AHT<xsl:text>stoqnka</xsl:text>
/POS22/AHT<xsl:text>wremq</xsl:text>
/POS31/AHT<xsl:text>pribytie</xsl:text>
/POS42/AHT<xsl:text>stoqnka</xsl:text>
/POS51/AHT<xsl:text>rasstoqnie,</xsl:text>
/POS63/AHT<xsl:text>period</xsl:text>
/POS73/AHT<xsl:text>glubina</xsl:text>
/CRLF
/POS22/AHT<xsl:text>w puti</xsl:text>
/POS55/AHT<xsl:text>km</xsl:text>
/POS60/AHT<xsl:text>kursirowaniq</xsl:text>
/POS73/AHT<xsl:text>prodavi</xsl:text>
/CRLF
<xsl:choose>
<xsl:when test="DEP/D3">
<xsl:value-of select="substring(DEP/D3, 9, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring(DEP/D3, 6, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring(DEP/D3, 1, 4)"/>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>/CRLF
<xsl:choose>
<xsl:when test="DEP/DW">
<xsl:value-of select="DEP/DW"/>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
<xsl:text> </xsl:text>
<xsl:choose>
<xsl:when test="DEP/T1">
<xsl:value-of select="substring(DEP/T1, 1, 5)"/>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
/CRLF
<xsl:choose>
<xsl:when test="(parent::*/NP/DLO)">
<xsl:call-template name="processDLO">
<xsl:with-param name="DLV" select="(parent::*/NP/DLO)"/>
</xsl:call-template>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
/CRLF
<xsl:choose>
<xsl:when test="VOK/C1">
<xsl:value-of select="VOK/C1"/>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
/POS06/RVTU
<xsl:choose>
<xsl:when test="DEP/T2">
<xsl:variable name="hours" select="number(substring(DEP/T2, 1, 2))"/>
<xsl:variable name="minutes" select="number(substring(DEP/T2, 4, 2))"/>
<xsl:choose>
<xsl:when test="$hours &gt; 0">
/POS13/AHT<xsl:value-of select="concat($hours, ' ~.')"/>
<xsl:choose>
<xsl:when test="$minutes &gt; 0">
/CRLF
/POS13/AHT<xsl:value-of select="concat($minutes, ' min')"/>/POS02/RVTU
</xsl:when>
</xsl:choose>
</xsl:when>
<xsl:otherwise>
<xsl:choose>
<xsl:when test="$minutes &gt; 0">
/POS13/AHT<xsl:value-of select="concat($minutes, ' min')"/>
</xsl:when>
<xsl:otherwise>
/POS13/AHT
</xsl:otherwise>
</xsl:choose>
</xsl:otherwise>
</xsl:choose>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
<xsl:choose>
<xsl:when test="T3">
/POS22/AHT<xsl:variable name="hours" select="number(substring-before(T3, ':'))"/>
<xsl:variable name="minutes" select="number(substring-before(substring-after(T3, ':'), ':'))"/>
<xsl:choose>
<xsl:when test="$hours &gt; 0">
/POS22/AHT<xsl:value-of select="concat($hours, ' ~. ')"/>/CRLF
<xsl:choose>
<xsl:when test="$minutes &gt; 0">
/POS22/AHT<xsl:value-of select="concat($minutes, ' min')"/>/POS02/RVTU
</xsl:when>
<xsl:otherwise>
/POS02/RVTU
</xsl:otherwise>
</xsl:choose>
</xsl:when>
<xsl:otherwise>
<xsl:choose>
<xsl:when test="$minutes &gt; 0">
/POS22/AHT<xsl:value-of select="concat($minutes, ' min')"/>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
</xsl:otherwise>
</xsl:choose>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
<xsl:choose>
<xsl:when test="ARR/D6">
/POS30/AHT
<xsl:value-of select="substring(ARR/D6, 9, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring(ARR/D6, 6, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring(ARR/D6, 1, 4)"/>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>/CRLF
<xsl:choose>
<xsl:when test="ARR/DW">
/POS30/AHT<xsl:value-of select="ARR/DW"/>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
<xsl:choose>
<xsl:when test="ARR/T4">/POS33/AHT
<xsl:value-of select="substring(ARR/T4, 1, 5)"/>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
/CRLF
/POS30/AHT
<xsl:choose>
<xsl:when test="(parent::*/NP/DLP)">
<xsl:call-template name="processDLP">
<xsl:with-param name="DLV1" select="(parent::*/NP/DLP)"/>
</xsl:call-template>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
/CRLF
<xsl:choose>
<xsl:when test="VOK/C2">/POS30/AHT
<xsl:value-of select="VOK/C2"/>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
<xsl:choose>
<xsl:when test="ARR/T4">
<xsl:choose>
<xsl:when test="substring(ARR/T4, 1, 2) = '00'">
/POS06/RVTU
</xsl:when>
<xsl:otherwise>
/POS06/RVTU
</xsl:otherwise>
</xsl:choose>
</xsl:when>
<xsl:otherwise>
/POS06/RVTU
</xsl:otherwise>
</xsl:choose>
<xsl:choose>
<xsl:when test="ARR/T2">/POS42/AHT
<xsl:variable name="hours" select="number(substring(ARR/T2, 1, 2))"/>
<xsl:variable name="minutes" select="number(substring(ARR/T2, 4, 2))"/>
<xsl:choose>
<xsl:when test="$hours &gt; 0">
/POS42/AHT<xsl:value-of select="concat($hours, ' ~.')"/>
<xsl:choose>
<xsl:when test="$minutes &gt; 0">
/CRLF
/POS42/AHT<xsl:value-of select="concat($minutes, ' min')"/>/POS02/RVTU
</xsl:when>
</xsl:choose>
</xsl:when>
<xsl:otherwise>
<xsl:choose>
<xsl:when test="$minutes &gt; 0">
/POS42/AHT<xsl:value-of select="concat($minutes, ' min')"/>
</xsl:when>
<xsl:otherwise>
/POS42/AHT
</xsl:otherwise>
</xsl:choose>
</xsl:otherwise>
</xsl:choose>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
/POS54/AHT<xsl:value-of select="L"/>
<xsl:choose>
<xsl:when test="INFOTR/DK or INFOTR/DW or INFOTR/RGL">
/POS60/AHT
<xsl:variable name="dkCount" select="count(INFOTR/DK)"/>
<xsl:variable name="dwCount" select="count(INFOTR/DW)"/>
<xsl:if test="INFOTR/DK">
<xsl:for-each select="INFOTR/DK">
<xsl:value-of select="substring(., 9, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring(., 6, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring(., 1, 4)"/>
<xsl:if test="position() != last()">
/CRLF/POS60/AHT
</xsl:if>
</xsl:for-each>
</xsl:if>
<xsl:if test="INFOTR/DK and INFOTR/DW">/CRLF
</xsl:if>
<xsl:if test="INFOTR/DW">
/POS60/AHT<xsl:text>po </xsl:text>
<xsl:for-each select="INFOTR/DW[position() &lt; 3]">
<xsl:value-of select="."/>
<xsl:if test="position() != last() or count(INFOTR/DW) &gt; 2">
<xsl:text>, </xsl:text>
</xsl:if>
</xsl:for-each>
<xsl:if test="count(INFOTR/DW) &gt; 2">
<xsl:text>,</xsl:text>
</xsl:if>
<xsl:for-each select="INFOTR/DW[position() &gt; 2]">
<xsl:if test="position() mod 3 = 1">
/CRLF/POS60/AHT
</xsl:if>
<xsl:value-of select="."/>
<xsl:if test="position() != last()">
<xsl:text>, </xsl:text>
</xsl:if>
</xsl:for-each>
</xsl:if>
<xsl:choose>
<xsl:when test="INFOTR/DK or INFOTR/DW">
<xsl:if test="INFOTR/RGL">
/CRLF/POS60/AHT<xsl:value-of select="INFOTR/RGL"/>
</xsl:if>
</xsl:when>
<xsl:when test="INFOTR/RGL">
/POS60/AHT<xsl:value-of select="INFOTR/RGL"/>
/CRLF
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
</xsl:when>
</xsl:choose>
<xsl:choose>
<xsl:when test="INFOTR/DK or INFOTR/DW or INFOTR/RGL">
/POS
<xsl:call-template name="NUM2POS">
<xsl:with-param name="NUM" select="((count(INFOTR/DW) &gt;= 1 and count(INFOTR/DW) &lt;= 2) * 0 + (count(INFOTR/DW) &gt;= 3 and count(INFOTR/DW) &lt;= 5) * 1 + (count(INFOTR/DW) &gt;= 6 and count(INFOTR/DW) &lt;= 7) * 2 + count(INFOTR/DK) + (boolean(INFOTR/RGL)) * 1)* 2"/>
</xsl:call-template>
/RVTU
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
/POS77/AHT<xsl:value-of select="KPP"/>
/POS06/RVTD
<xsl:if test="VOK/C1 or VOK/C2">
/POS02/RVTD
</xsl:if>
<xsl:choose>
<xsl:when test="VOK/C1 or VOK/C2">
<xsl:variable name="LC" select="count(INFOTR/DK)"/>
<xsl:variable name="DC" select="count(INFOTR/DW)"/>
<xsl:variable name="RC" select="count(INFOTR/RGL)"/>
<xsl:variable name="DV">
<xsl:choose>
<xsl:when test="$DC &lt;= 2">1</xsl:when>
<xsl:when test="$DC &gt;= 3 and $DC &lt;= 5">2</xsl:when>
<xsl:when test="$DC &gt;= 6 and $DC &lt;= 7">3</xsl:when>
<xsl:otherwise>0</xsl:otherwise>
</xsl:choose>
</xsl:variable>
<xsl:variable name="total" select="($LC + $DV + $RC)"/>
<xsl:variable name="result" select="($total - 4) * 2"/>
<xsl:if test="$result &gt; 0">
<xsl:choose>
<xsl:when test="INFOTR/DK or INFOTR/DW or INFOTR/RGL">
/POS
<xsl:call-template name="NUM2POS">
<xsl:with-param name="NUM" select="$result"/>
</xsl:call-template>
/RVTD
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
</xsl:if>
</xsl:when>
<xsl:otherwise>
<xsl:variable name="LC" select="count(INFOTR/DK)"/>
<xsl:variable name="DC" select="count(INFOTR/DW)"/>
<xsl:variable name="RC" select="count(INFOTR/RGL)"/>
<xsl:variable name="DV">
<xsl:choose>
<xsl:when test="$DC &lt;= 2">1</xsl:when>
<xsl:when test="$DC &gt;= 3 and $DC &lt;= 5">2</xsl:when>
<xsl:when test="$DC &gt;= 6 and $DC &lt;= 7">3</xsl:when>
<xsl:otherwise>0</xsl:otherwise>
</xsl:choose>
</xsl:variable>
<xsl:variable name="total" select="($LC + $DV + $RC)"/>
<xsl:variable name="result" select="($total - 3) * 2"/>
<xsl:if test="$result &gt; 0">
<xsl:choose>
<xsl:when test="INFOTR/DK or INFOTR/DW or INFOTR/RGL">
/POS
<xsl:call-template name="NUM2POS">
<xsl:with-param name="NUM" select="$result"/>
</xsl:call-template>
/RVTD
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
</xsl:if>
</xsl:otherwise>
</xsl:choose>
</xsl:template>
<xsl:template name="ZAGOLOVOK1">
/POS19/AHT<xsl:text>sprawo~naq informaciq o raspisanii poezdow</xsl:text>/CRLF
/POS19/AHT<xsl:text>ot stancii </xsl:text><xsl:value-of select="NP/C1"/><xsl:text> do stancii </xsl:text>
<xsl:value-of select="NP/C2"/>/CRLF
/POS19/AHT<xsl:text>data otprawleniq </xsl:text>
<xsl:value-of select="substring(D1, 9, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring(D1, 6, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring(D1, 1, 4)"/>
/CRLF
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
<xsl:when test="RESP/@TYPE ='SPR_TSCHEDULE'">
<xsl:choose>
<xsl:when test="@W='80' and @H='20' and substring(@T,1,1)='S'">
<xsl:apply-templates mode="T8020" select="RESP"/>
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
