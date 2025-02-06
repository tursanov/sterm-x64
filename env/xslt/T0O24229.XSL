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
<xsl:template name="NUM2POS">
<xsl:param name="NUM"/>
<xsl:variable name="POS" select="concat('00',$NUM)"/>
<xsl:value-of select="substring($POS,string-length($POS)-1)"/>
</xsl:template>
<xsl:template name="CENTER">
 <xsl:param name="W"/>
 <xsl:param name="V"/>
/POS
  <xsl:variable name="REM" select="number($W - string-length($V))"/>
  <xsl:choose>
    <xsl:when test="$REM &gt; 0">
      <xsl:choose>
        <xsl:when test="($REM mod 2) = 0">
           <xsl:call-template name="NUM2POS">
             <xsl:with-param name="NUM" select="$REM div 2"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="NUM2POS">
             <xsl:with-param name="NUM" select="($REM - 1) div 2"/>
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
      <xsl:text>00</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
/AHT
<xsl:value-of select="$V"/>
</xsl:template>
<xsl:template mode="T8020" match="RESP">
<xsl:apply-templates select="INFOEX"/>
/CRLF
<xsl:apply-templates select="ZG"/>
/CRLF
</xsl:template>
<xsl:template match="INFOEX">
/POS00/AHT<xsl:text>data prowedeniq</xsl:text>
/POS16/AHT<xsl:call-template name="YMD2DMY">
   <xsl:with-param name="YMD" select="D3"/>
   </xsl:call-template>
/CRLF
/POS00/AHT<xsl:text>nomer |kskursii</xsl:text>
/POS28/AHT<xsl:text>nazwanie |kskursii</xsl:text>
/POS61/AHT<xsl:text>stanciq prowedeniq</xsl:text>
/CRLF
/POS00/AHT<xsl:value-of select="NEX"/>
      <xsl:choose>
        <xsl:when test="string-length(NAMEEX) &gt; 50">
/POS09/AHT<xsl:value-of select="substring(NAMEEX,1,50)"/>
/CRLF
/POS09/AHT<xsl:value-of select="substring(NAMEEX,51)"/>
/POS02/RVTU
        </xsl:when>
        <xsl:otherwise>
/POS09/AHT<xsl:value-of select="NAMEEX"/>
        </xsl:otherwise>
      </xsl:choose>
/POS60/AHT<xsl:value-of select="CC"/>
/POS68/AHT<xsl:value-of select="C"/>
/CRLF
<xsl:if test="string-length(NAMEEX) &gt; 50">
/CRLF
</xsl:if>
/POS00/AHT<xsl:text>dostupnye warianty</xsl:text>
/CRLF
/CRLF
/POS00/AHT<xsl:text>nomer</xsl:text>
/POS12/AHT<xsl:text>wremq</xsl:text>
/POS19/AHT<xsl:text>wremq</xsl:text>
/POS31/AHT<xsl:text>wid tarifa</xsl:text>
/POS48/AHT<xsl:text>ob}aq</xsl:text>
/POS67/AHT<xsl:text>dostupno kwot</xsl:text>
/CRLF
/POS00/AHT<xsl:text>warianta</xsl:text>
/POS12/AHT<xsl:text>na~ala</xsl:text>
/POS19/AHT<xsl:text>okon~aniq</xsl:text>
/POS47/AHT<xsl:text>stoimostx</xsl:text>
/CRLF
/CRLF
<xsl:apply-templates select="INFOVAR"/>
</xsl:template>
<xsl:template match="INFOVAR">
/POS00/AHT<xsl:value-of select="VAREX"/>
/POS12/AHT<xsl:value-of select="substring(T1,1,5)"/>
/POS19/AHT<xsl:value-of select="substring(T4,1,5)"/>
<xsl:apply-templates select="INFOVD"/>
      <xsl:choose>
        <xsl:when test="string-length(NAMEVAR) &gt; 80">
/POS00/AHT<xsl:value-of select="substring(NAMEVAR,1,80)"/>
/CRLF
/POS00/AHT<xsl:value-of select="substring(NAMEVAR,81)"/>
        </xsl:when>
        <xsl:otherwise>
/POS00/AHT<xsl:value-of select="NAMEVAR"/>
        </xsl:otherwise>
      </xsl:choose>
/CRLF
/POS00/AHT<xsl:text>----------</xsl:text>
<xsl:text>----------</xsl:text>
<xsl:text>----------</xsl:text>
<xsl:text>----------</xsl:text>
<xsl:text>----------</xsl:text>
<xsl:text>----------</xsl:text>
<xsl:text>----------</xsl:text>
<xsl:text>----------</xsl:text>
/CRLF
</xsl:template>
<xsl:template match="INFOVD">
/POS31/AHT<xsl:value-of select="VD"/>
/POS47/AHT<xsl:value-of select="SE"/>
/POS68/AHT<xsl:value-of select="KOLS"/>
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
<xsl:when test="RESP/@TYPE ='SPR_EXCUR'">
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
