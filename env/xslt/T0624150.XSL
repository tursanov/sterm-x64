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
<xsl:call-template name="TITLE"/>
<xsl:apply-templates mode="DEP" select="DEP/TRAINS"/>
<xsl:apply-templates mode="TRANSIT" select="TRANSIT/TRAINS"/>
<xsl:apply-templates mode="ARR" select="ARR/TRAINS"/>
</xsl:template>
<xsl:template name="TITLE">
/POS12/AHT<xsl:text>sprawo~naq informaciq o raspisanii dwiveniq poezdow</xsl:text>/CRLF
/POS16/AHT<xsl:text>po stancii </xsl:text><xsl:value-of select="C"/>
<xsl:text> na </xsl:text><xsl:value-of select="concat(substring(D1,9,2),'.',substring(D1,6,2),'.',substring(D1,1,4))"/>/CRLF
</xsl:template>
<xsl:template name="TITLE_DEP">
/POS34/AHT<xsl:text>otprawlenie</xsl:text>/CRLF
/POS00/AHT<xsl:text>nomer</xsl:text>
/POS09/AHT<xsl:text>mar{rut sledowaniq</xsl:text>
/POS33/AHT<xsl:text>stoqnka</xsl:text>
/POS43/AHT<xsl:text>wremq</xsl:text>
/POS53/AHT<xsl:text>wremq</xsl:text>
/POS65/AHT<xsl:text>wremq w</xsl:text>
/POS74/AHT<xsl:text>doroga</xsl:text>/CRLF
/POS00/AHT<xsl:text>poezda</xsl:text>
/POS41/AHT<xsl:text>otprawleniq</xsl:text>
/POS53/AHT<xsl:text>pribytiq na</xsl:text>
/POS65/AHT<xsl:text>puti</xsl:text>/CRLF
/POS53/AHT<xsl:text>stanci`</xsl:text>/CRLF
/POS53/AHT<xsl:text>nazna~eniq</xsl:text>/CRLF
/POS49/AHT<xsl:text>regulqrnostx kursirowaniq</xsl:text>/CRLF
</xsl:template>
<xsl:template name="TITLE_TRANSIT">
/POS34/AHT<xsl:text>tranzit</xsl:text>/CRLF
/POS00/AHT<xsl:text>nomer</xsl:text>
/POS09/AHT<xsl:text>mar{rut sledowaniq</xsl:text>
/POS33/AHT<xsl:text>stoqnka</xsl:text>
/POS43/AHT<xsl:text>wremq</xsl:text>
/POS53/AHT<xsl:text>wremq</xsl:text>
/POS65/AHT<xsl:text>wremq w</xsl:text>
/POS74/AHT<xsl:text>doroga</xsl:text>/CRLF
/POS00/AHT<xsl:text>poezda</xsl:text>
/POS41/AHT<xsl:text>otprawleniq</xsl:text>
/POS53/AHT<xsl:text>pribytiq na</xsl:text>
/POS65/AHT<xsl:text>puti</xsl:text>/CRLF
/POS53/AHT<xsl:text>stanci`</xsl:text>/CRLF
/POS53/AHT<xsl:text>nazna~eniq</xsl:text>/CRLF
/POS49/AHT<xsl:text>regulqrnostx kursirowaniq</xsl:text>/CRLF
</xsl:template>
<xsl:template name="TITLE_ARR">
/POS34/AHT<xsl:text>pribytie</xsl:text>/CRLF
/POS00/AHT<xsl:text>nomer</xsl:text>
/POS09/AHT<xsl:text>mar{rut sledowaniq</xsl:text>
/POS33/AHT<xsl:text>stoqnka</xsl:text>
/POS43/AHT<xsl:text>wremq</xsl:text>
/POS53/AHT<xsl:text>wremq</xsl:text>
/POS65/AHT<xsl:text>wremq w</xsl:text>
/POS74/AHT<xsl:text>doroga</xsl:text>/CRLF
/POS00/AHT<xsl:text>poezda</xsl:text>
/POS41/AHT<xsl:text>otprawleniq</xsl:text>
/POS53/AHT<xsl:text>pribytiq na</xsl:text>
/POS65/AHT<xsl:text>puti</xsl:text>/CRLF
/POS41/AHT<xsl:text>s na~alxnoj</xsl:text>/CRLF
/POS41/AHT<xsl:text>stancii</xsl:text>/CRLF
/POS49/AHT<xsl:text>regulqrnostx kursirowaniq</xsl:text>/CRLF
</xsl:template>
<xsl:template mode="DEP" match="TRAINS">
<xsl:call-template name="TITLE_DEP"/>
<xsl:apply-templates select="PP">
<xsl:with-param name="P1" select="'DEP_TRANSIT'"/>
</xsl:apply-templates>
</xsl:template>
<xsl:template mode="TRANSIT" match="TRAINS">
<xsl:call-template name="TITLE_TRANSIT"/>
<xsl:apply-templates select="PP">
<xsl:with-param name="P1" select="'DEP_TRANSIT'"/>
</xsl:apply-templates>
</xsl:template>
<xsl:template mode="ARR" match="TRAINS">
<xsl:call-template name="TITLE_ARR"/>
<xsl:apply-templates select="PP">
<xsl:with-param name="P1" select="'ARR'"/>
</xsl:apply-templates>
</xsl:template>
<xsl:template match="PP">
<xsl:param name="P1"/>
<xsl:call-template name="IV">
<xsl:with-param name="P1" select="$P1"/>
<xsl:with-param name="P2" select="N1"/>
<xsl:with-param name="P3" select="Z1"/>
</xsl:call-template>
<xsl:apply-templates select="CARS">
<xsl:with-param name="P1" select="$P1"/>
<xsl:with-param name="P2" select="N1"/>
</xsl:apply-templates>
</xsl:template>
<xsl:template match="CARS">
<xsl:param name="P1"/>
<xsl:param name="P2"/>
<xsl:call-template name="IV">
<xsl:with-param name="P1" select="$P1"/>
<xsl:with-param name="P2" select="'wagony'"/>
<xsl:with-param name="P3" select="VP"/>
</xsl:call-template>
</xsl:template>
<xsl:template name="IV">
<xsl:param name="P1"/>
<xsl:param name="P2"/>
<xsl:param name="P3"/>
/POS00/AHT<xsl:value-of select="$P2"/>
/POS09/AHT<xsl:value-of select="$P3/C1"/>
<xsl:text> (</xsl:text>
<xsl:value-of select="$P3/CC3"/>
<xsl:text>)-</xsl:text>
<xsl:choose>
<xsl:when test="not (T2)">
/POS34/AHT<xsl:text>-</xsl:text>
</xsl:when>
<xsl:when test="(substring(T2,1,2)='00') and (substring(T2,4,1)='0')">
/POS34/AHT<xsl:value-of select="substring(T2,5,1)"/>
<xsl:text> min</xsl:text>
</xsl:when>
<xsl:when test="(substring(T2,1,2)='00') and (substring(T2,4,1)!='0')">
/POS34/AHT<xsl:value-of select="substring(T2,4,2)"/>
<xsl:text> min</xsl:text>
</xsl:when>
<xsl:when test="substring(T2,1,1)='0'">
/POS34/AHT<xsl:value-of select="substring(T2,2,1)"/>
<xsl:text> ~ </xsl:text><xsl:value-of select="substring(T2,4,2)"/>
<xsl:text> min</xsl:text>
</xsl:when>
<xsl:otherwise>
/POS34/AHT<xsl:value-of select="substring(T2,1,2)"/>
<xsl:text> ~ </xsl:text><xsl:value-of select="substring(T2,4,2)"/>
<xsl:text> min</xsl:text>
</xsl:otherwise>
</xsl:choose>
<xsl:choose>
<xsl:when test="$P1='DEP_TRANSIT'">
/POS43/AHT<xsl:value-of select="substring(T7,1,5)"/>
/POS53/AHT<xsl:value-of select="substring(T4,1,5)"/>
</xsl:when>
<xsl:when test="$P1='ARR'">
/POS43/AHT<xsl:value-of select="substring(T5,1,5)"/>
/POS53/AHT<xsl:value-of select="substring(T6,1,5)"/>
</xsl:when>
<xsl:otherwise>
<xsl:text>neprawilxnoe zna~enie parametra </xsl:text> <xsl:value-of select="$P1"/>
</xsl:otherwise>
</xsl:choose>
/POS65/AHT<xsl:value-of select="substring(T3,1,5)"/>
/POS74/AHT<xsl:value-of select="W2"/>/CRLF
/POS09/AHT<xsl:value-of select="$P3/C2"/>
<xsl:text> (</xsl:text>
<xsl:value-of select="$P3/CC4"/>
<xsl:text>)</xsl:text>
<xsl:choose>
<xsl:when test=" not (DK)">
/POS49/AHT<xsl:value-of select="RGL"/>
</xsl:when>
<xsl:when test="string-length(DK)=10">
/POS49/AHT<xsl:text>po </xsl:text><xsl:value-of select="concat(substring(DK, 9,2),'.',substring(DK, 6,2))"/>
<xsl:if test="RGL">
<xsl:text> </xsl:text><xsl:value-of select="RGL"/>
</xsl:if>
</xsl:when>
<xsl:when test="(string-length(DK) &gt; 10) and not (RGL)">
/POS49/AHT<xsl:value-of select="DK"/>
</xsl:when>
<xsl:otherwise>
/POS49/AHT<xsl:text>neopisannaq kombinaciq uzlow DK (</xsl:text><xsl:value-of select="DK"/>
<xsl:text>) i RGL (</xsl:text>
<xsl:value-of select="RGL"/><xsl:text>)</xsl:text>
</xsl:otherwise>
</xsl:choose>
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
<xsl:when test="RESP/@TYPE ='SPR_SCHEDULESTATION'">
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
