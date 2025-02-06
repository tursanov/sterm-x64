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
/POS22/AHT<xsl:text>sprawo~naq informaciq o kurse wal`t</xsl:text>
/CRLF
<xsl:apply-templates select="LR"/>
</xsl:template>
<xsl:template match="LR">
/POS03/AHT<xsl:text>summa</xsl:text>
/POS18/AHT<xsl:text>iz wal`ty</xsl:text>
/POS35/AHT<xsl:text>summa</xsl:text>
/POS50/AHT<xsl:text>w wal`tu</xsl:text>
/POS66/AHT<xsl:text>data</xsl:text>
/POS73/AHT<xsl:text>data</xsl:text>
/CRLF
/POS01/AHT<xsl:text>perewoda</xsl:text>
<xsl:if test="@GOSNS">/POS10/AHT<xsl:value-of select="concat(substring('          ',1,(20-string-length(@GOSNS)) div 2),'(',@GOSNS,')')"/></xsl:if>
/POS33/AHT<xsl:text>w wal`te</xsl:text>
<xsl:if test="@GOSND">/POS42/AHT<xsl:value-of select="concat(substring('          ',1,(20-string-length(@GOSND)) div 2),'(',@GOSND,')')"/></xsl:if>
/POS65/AHT<xsl:text>na~ala</xsl:text>
/POS73/AHT<xsl:text>konca</xsl:text>
/CRLF
<xsl:apply-templates select="RATE">
<xsl:with-param name="QVU" select="@QVU"/>
<xsl:with-param name="CN" select="@CN"/>
<xsl:with-param name="NVC" select="@NVC"/>
<xsl:with-param name="VS1K" select="VS1K"/>
<xsl:with-param name="VD1K" select="VD1K"/>
</xsl:apply-templates>
</xsl:template>
<xsl:template match="RATE">
<xsl:param name="QVU"/>
<xsl:param name="CN"/>
<xsl:param name="NVC"/>
<xsl:param name="VS1K"/>
<xsl:param name="VD1K"/>
/POS00/AHT<xsl:value-of select="$QVU"/>
<xsl:if test="$VS1K"><xsl:text> tys</xsl:text>
</xsl:if>
/POS21/AHT<xsl:value-of select="$CN"/>
/POS32/AHT<xsl:value-of select="@SNV"/>
<xsl:if test="$VD1K"><xsl:text> tys</xsl:text>
</xsl:if>
/POS52/AHT<xsl:value-of select="$NVC"/>
/POS65/AHT<xsl:value-of select="concat(substring(@DATAN,9,2),'.',substring(@DATAN,6,2))"/>
/POS73/AHT<xsl:value-of select="concat(substring(@DATAK,9,2),'.',substring(@DATAK,6,2))"/>
/CRLF
/POS66/AHT<xsl:value-of select="substring(@DATAN,1,4)"/>
/POS74/AHT<xsl:value-of select="substring(@DATAK,1,4)"/>
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
<xsl:when test="RESP/@TYPE ='SPR_EXCHANGERATE'">
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
