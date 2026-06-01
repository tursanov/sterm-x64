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
<xsl:template mode="T8020" match="RESP">
/POS04/AHT<xsl:text>sprawo~naq informaciq o serwisnyh uslugah, predostawlqemyh w poezde </xsl:text>
<xsl:value-of select="N1"/>/CRLF
/POS28/AHT<xsl:text>otprawleniem </xsl:text>
<xsl:value-of select="concat(substring(D3,9,2),'.',substring(D3,6,2),'.',substring(D3,1,4))"/>
/CRLF
/POS00/AHT<xsl:text>kod</xsl:text>
/POS07/AHT<xsl:text>nomer</xsl:text>
/POS13/AHT<xsl:text>perewoz~ik</xsl:text>
/POS29/AHT<xsl:text>tip</xsl:text>
/POS33/AHT<xsl:text>klass</xsl:text>
/POS54/AHT<xsl:text>opisanie uslugi</xsl:text>
/CRLF
/POS00/AHT<xsl:text>nabora</xsl:text>
/POS07/AHT<xsl:text>uslu-</xsl:text>
/POS29/AHT<xsl:text>wa-</xsl:text>
/POS33/AHT<xsl:text>obs-</xsl:text>
/CRLF
/POS00/AHT<xsl:text>ser-</xsl:text>
/POS07/AHT<xsl:text>gi</xsl:text>
/POS29/AHT<xsl:text>go</xsl:text>
/POS33/AHT<xsl:text>luvi-</xsl:text>
/CRLF
/POS00/AHT<xsl:text>wisnyh</xsl:text>
/POS29/AHT<xsl:text>na</xsl:text>
/POS33/AHT<xsl:text>waniq</xsl:text>
/CRLF
/POS00/AHT<xsl:text>uslug</xsl:text>
/CRLF
<xsl:apply-templates select="INFONU"/>
</xsl:template>
<xsl:template match="INFONU">
<xsl:apply-templates select="INFOUS">
<xsl:with-param name="NU" select="NU"/>
<xsl:with-param name="VB" select="VB"/>
<xsl:with-param name="KV" select="KV"/>
<xsl:with-param name="KL" select="KL"/>
</xsl:apply-templates>
</xsl:template>
<xsl:template name="NUM2POS">
<xsl:param name="NUM"/>
<xsl:variable name="POS" select="concat('00',$NUM)"/>
<xsl:value-of select="substring($POS,string-length($POS)-1)"/>
</xsl:template>
<xsl:template name="DOWNIFMORE">
<xsl:param name="NL1"/>
<xsl:param name="NL2"/>
<xsl:if test="$NL1 &gt; $NL2">
/POS
<xsl:call-template name="NUM2POS">
<xsl:with-param name="NUM" select="2 * ($NL1 - $NL2)"/>
</xsl:call-template>
/RVTD
</xsl:if>
</xsl:template>
<xsl:template match="INFOUS">
<xsl:param name="NU"/>
<xsl:param name="VB"/>
<xsl:param name="KV"/>
<xsl:param name="KL"/>
/POS00/AHT<xsl:value-of select="$NU"/>
/POS07/AHT<xsl:value-of select="NUS"/>
/POS13/AHT<xsl:value-of select="$VB"/>
/POS29/AHT<xsl:value-of select="$KV"/>
<xsl:for-each select="$KL">
<xsl:if test="(position() mod 3) = 1">
/POS33/AHT
</xsl:if>
<xsl:value-of select="."/>
<xsl:if test="position()!=last()">
<xsl:text>,</xsl:text>
</xsl:if>
<xsl:if test="(position() mod 3) = 0">
/CRLF
</xsl:if>
</xsl:for-each>
<xsl:variable name="NUMCRLFINKLLINES" select="(count($KL)-(count($KL) mod 3)) div 3"/>
<xsl:if test="$NUMCRLFINKLLINES &gt; 0">
/POS
<xsl:call-template name="NUM2POS">
<xsl:with-param name="NUM" select="2*$NUMCRLFINKLLINES"/>
</xsl:call-template>
/RVTU
</xsl:if>
<xsl:choose>
<xsl:when test="string-length(OPUS) &lt; 38">
/POS43/AHT<xsl:value-of select="substring(OPUS,1,37)"/>/CRLF
<xsl:call-template name="DOWNIFMORE">
<xsl:with-param name="NL1" select="$NUMCRLFINKLLINES + 1"/>
<xsl:with-param name="NL2" select="1"/>
</xsl:call-template>
</xsl:when>
<xsl:when test="(string-length(OPUS) &gt; 37) and (string-length(OPUS) &lt; 75)">
/POS43/AHT<xsl:value-of select="substring(OPUS,1,37)"/>/CRLF
/POS43/AHT<xsl:value-of select="substring(OPUS,38,37)"/>/CRLF
<xsl:call-template name="DOWNIFMORE">
<xsl:with-param name="NL1" select="$NUMCRLFINKLLINES + 1"/>
<xsl:with-param name="NL2" select="2"/>
</xsl:call-template>
</xsl:when>
<xsl:otherwise>
/POS43/AHT<xsl:value-of select="substring(OPUS,1,37)"/>/CRLF
/POS43/AHT<xsl:value-of select="substring(OPUS,38,37)"/>/CRLF
/POS43/AHT<xsl:value-of select="substring(OPUS,75,37)"/>/CRLF
<xsl:call-template name="DOWNIFMORE">
<xsl:with-param name="NL1" select="$NUMCRLFINKLLINES + 1"/>
<xsl:with-param name="NL2" select="3"/>
</xsl:call-template>
</xsl:otherwise>
</xsl:choose>
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
<xsl:when test="RESP/@TYPE ='SPR_SERVICE'">
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
