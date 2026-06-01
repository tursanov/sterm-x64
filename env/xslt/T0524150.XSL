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
<xsl:apply-templates mode="DP" select=".">
<xsl:with-param name="MODE" select="'D'"/>
</xsl:apply-templates>
</xsl:template>
<xsl:template mode="P" match="RESP">
<xsl:text disable-output-escaping="yes">/ESC{/ESC!8003030/ESC#/POS02/AVT</xsl:text>
<xsl:text disable-output-escaping="yes">/CPI20</xsl:text>
<xsl:apply-templates mode="DP" select=".">
<xsl:with-param name="MODE" select="'P'"/>
</xsl:apply-templates>
<xsl:text disable-output-escaping="yes">/FF</xsl:text>
</xsl:template>
<xsl:template mode="DP" match="RESP">
<xsl:param name="MODE"/>
<xsl:choose>
<xsl:when test="$MODE='D'">/POS00/AHT</xsl:when>
<xsl:otherwise>/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:choose>
<xsl:when test="BRDST = '3'">
<xsl:value-of select="'registraciq posadki w poezd'"/>
</xsl:when>
<xsl:when test="BRDST = '4'">
<xsl:value-of select="'otmena registracii posadki w poezd'"/>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
/CRLF
<xsl:value-of select="N1"/>
<xsl:text> </xsl:text>
<xsl:value-of select="Z1/C1"/>
<xsl:text> - </xsl:text>
<xsl:value-of select="Z1/C2"/>
/CRLF
<xsl:text>data otprawleniq poezda </xsl:text>
<xsl:variable name="year" select="substring(D1, 1, 4)"/>
<xsl:variable name="month" select="substring(D1, 6, 2)"/>
<xsl:variable name="day" select="substring(D1, 9, 2)"/>
<xsl:value-of select="concat($day, '.', $month, '.', $year)"/>
/CRLF
<xsl:text>dokumentow </xsl:text>
<xsl:value-of select="KOL"/>
/CRLF
/CRLF
<xsl:choose>
<xsl:when test="$MODE='D'">/POS00/AHT</xsl:when>
<xsl:otherwise>/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:text>nomer dokumenta</xsl:text>
<xsl:choose>
<xsl:when test="$MODE='D'">/POS18/AHT</xsl:when>
<xsl:otherwise>/POS18/AHT</xsl:otherwise>
</xsl:choose>
<xsl:text>status</xsl:text>
<xsl:choose>
<xsl:when test="$MODE='D'">/POS33/AHT</xsl:when>
<xsl:otherwise>/POS33/AHT</xsl:otherwise>
</xsl:choose>
<xsl:text>pri~ina</xsl:text>
/CRLF
<xsl:choose>
<xsl:when test="$MODE='D'"/>
<xsl:otherwise><xsl:text disable-output-escaping="yes">/NAK/ESC{/ESC!8003030/ESC#/POS02/AVT</xsl:text>
<xsl:text disable-output-escaping="yes">/CPI20</xsl:text>
</xsl:otherwise>
</xsl:choose>
<xsl:for-each select="PRZD">
<xsl:choose>
<xsl:when test="$MODE='D'">/POS00/AHT</xsl:when>
<xsl:otherwise>/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:choose>
<xsl:when test="not (@NB)">
</xsl:when>
<xsl:when test="not (string-length(@NB)=14 and translate(@NB, '0123456789', '')='')">
<xsl:value-of select="substring(@NB, 1, 14)"/>
</xsl:when>
<xsl:otherwise>
<xsl:value-of select="@NB"/>
</xsl:otherwise>
</xsl:choose>
<xsl:choose>
<xsl:when test="$MODE='D'">/POS20/AHT</xsl:when>
<xsl:otherwise>/POS20/AHT</xsl:otherwise>
</xsl:choose>
<xsl:choose>
<xsl:when test="not (@K0)">
</xsl:when>
<xsl:when test="@K0 = '0'">
<xsl:text>da</xsl:text>
</xsl:when>
<xsl:when test="@K0 = '1'">
<xsl:text>net</xsl:text>
<xsl:choose>
<xsl:when test="$MODE='D'">/POS26/AHT</xsl:when>
<xsl:otherwise>/POS26/AHT</xsl:otherwise>
</xsl:choose>
<xsl:choose>
<xsl:when test="not (@K1)">
</xsl:when>
<xsl:when test="@K1 = '1'">
<xsl:text>poga{en</xsl:text>
</xsl:when>
<xsl:when test="@K1 = '2'">
<xsl:text>wozwra}en</xsl:text>
</xsl:when>
<xsl:when test="@K1 = '3'">
<xsl:text>~asti~nyj wozwrat</xsl:text>
</xsl:when>
<xsl:when test="@K1 = '4'">
<xsl:text>wozwra}eny mesta</xsl:text>
</xsl:when>
<xsl:when test="@K1 = '5'">
<xsl:text>pereoformlen</xsl:text>
</xsl:when>
<xsl:when test="@K1 = '6'">
<xsl:text>wydan dublikat ili bumavnaq kopiq</xsl:text>
</xsl:when>
<xsl:when test="@K1 = '7'">
<xsl:text>prerywanie poezdki</xsl:text>
</xsl:when>
<xsl:when test="@K1 = '8'">
<xsl:text>|b bez |r</xsl:text>
</xsl:when>
<xsl:when test="@K1 = '9'">
<xsl:text>posadka byla ranee</xsl:text>
</xsl:when>
<xsl:when test="@K1 = 'A'">
<xsl:text>posadki ne bylo</xsl:text>
</xsl:when>
<xsl:when test="@K1 = 'B'">
<xsl:text>dokument ne najden</xsl:text>
</xsl:when>
<xsl:when test="@K1 = 'C'">
<xsl:text>sintaksi~eskaq o{ibka</xsl:text>
</xsl:when>
<xsl:when test="@K1 = 'D'">
<xsl:text>informaciq nedostupna</xsl:text>
</xsl:when>
<xsl:when test="@K1 = 'E'">
<xsl:text>ne sowpali poezd i data</xsl:text>
</xsl:when>
<xsl:when test="@K1 = '*'">
<xsl:text>oplaty ne bylo</xsl:text>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
/CRLF
<xsl:choose>
<xsl:when test="$MODE='D'"/>
<xsl:otherwise>
<xsl:choose>
<xsl:when test="(position()!=last()) and ((position() mod 53)=0)">/NAK/ESC{/ESC!8003030/ESC#/POS02/AVT/CPI20</xsl:when>
</xsl:choose>
</xsl:otherwise>
</xsl:choose>
</xsl:for-each>
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
<xsl:when test="RESP/@TYPE ='BOARDINGPASS'">
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
