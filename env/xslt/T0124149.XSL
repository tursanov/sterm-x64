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
<xsl:template match="ZG">
<xsl:value-of select="substring(@DZ, 9, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring(@DZ, 6, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring(@DZ, 1, 4)"/>
<xsl:text> </xsl:text>
<xsl:choose>
<xsl:when test="@TZ">
<xsl:variable name="hours" select="substring(@TZ, 1, 2)"/>
<xsl:variable name="minutes" select="substring(@TZ, 4, 2)"/>
<xsl:variable name="seconds" select="substring(@TZ, 7, 2)"/>
<xsl:value-of select="concat($hours, ':', $minutes, ':', $seconds)"/>
<xsl:text> </xsl:text>
</xsl:when>
<xsl:otherwise>
<xsl:text>ne ukazano wremq </xsl:text>
</xsl:otherwise>
</xsl:choose>
<xsl:value-of select="IZK"/>
<xsl:text> </xsl:text>
<xsl:value-of select="TM/@VC"/>
<xsl:value-of select="TM/NPP"/>
<xsl:value-of select="TM/SDOR"/>
<xsl:value-of select="TM/PNTM"/>
</xsl:template>
<xsl:template mode="T8020" match="RESP">
<xsl:choose>
<xsl:when test="count(ST) &gt; 0">
/POS11/AHT<xsl:text>sprawo~naq informaciq o kodah gosudarstw</xsl:text>/CRLF
/POS11/AHT<xsl:apply-templates select="ZG"/>/CRLF
/CRLF
<xsl:text>/POS04/AHTkod
/POS15/AHTkod v.d.
/POS27/AHTnazwanie gosudarstwa</xsl:text>
/CRLF
/POS00/AHTgosudarstwa
/POS12/AHTadministracii
/CRLF
<xsl:apply-templates mode="T8020" select="ST"/>
</xsl:when>
<xsl:otherwise>
/POS16/AHT<xsl:text>net informacii o gosudarstwah</xsl:text>/CRLF
/POS11/AHT<xsl:apply-templates select="ZG"/>/CRLF
</xsl:otherwise>
</xsl:choose>
</xsl:template>
<xsl:template mode="T8020" match="ST">
/POS04/AHT<xsl:value-of select="@GR"/>
/POS17/AHT<xsl:value-of select="@GP"/>
/POS27/AHT<xsl:value-of select="@GN"/>
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
<xsl:when test="RESP/@TYPE ='SPR_BISC'">
<xsl:choose>
<xsl:when test="@W='80' and @H='20' and substring(@T, 1, 1)='S'">
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
