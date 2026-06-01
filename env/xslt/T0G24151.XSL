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
<xsl:template name="ZAGOLOVOK">
<xsl:text>sprawo~naq informaciq o sheme sostawa poezda </xsl:text>
<xsl:value-of select="PP/N1"/>
<xsl:text> otprawleniem </xsl:text>
<xsl:value-of select="substring(PP/D1, 9, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring(PP/D1, 6, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring(PP/D1, 1, 4)"/>
/CRLF
<xsl:apply-templates select="ZG"/>
/CRLF
/CRLF
<xsl:text>otkrytie prodavi za </xsl:text>
<xsl:variable name="lastTwoDigits" select="number(substring(PP/KPP, string-length(PP/KPP) - 1))"/>
<xsl:variable name="lastDigit" select="number(substring(PP/KPP, string-length(PP/KPP)))"/>
<xsl:choose>
<xsl:when test="($lastTwoDigits &gt;= 11 and $lastTwoDigits &lt;= 20) or (($lastDigit &gt;= 5) or $lastDigit = 0)">
<xsl:value-of select="PP/KPP"/>
<xsl:text> dnej</xsl:text>
</xsl:when>
<xsl:when test="$lastDigit = 1">
<xsl:value-of select="PP/KPP"/>
<xsl:text> denx</xsl:text>
</xsl:when>
<xsl:otherwise>
<xsl:value-of select="PP/KPP"/>
<xsl:text> dnq</xsl:text>
</xsl:otherwise>
</xsl:choose>
<xsl:text>, poezd otprawlqetsq hwostom wpered</xsl:text>
/CRLF
/CRLF
<xsl:text>wagony:</xsl:text>
/CRLF
/POS00/AHT<xsl:text>N</xsl:text>
/POS03/AHT<xsl:text>tip</xsl:text>
/POS07/AHT<xsl:text>kl</xsl:text>
/POS10/AHT<xsl:text>podtip</xsl:text>
/POS17/AHT<xsl:text>rod</xsl:text>
/POS21/AHT<xsl:text>perewoz~ik</xsl:text>
/POS34/AHT<xsl:text>punkty</xsl:text>
/POS47/AHT<xsl:text>mesta</xsl:text>
/POS63/AHT<xsl:text>priznaki</xsl:text>
/CRLF
/POS07/AHT<xsl:text>obs</xsl:text>
/POS22/AHT<xsl:text>i doroga</xsl:text>
/POS33/AHT<xsl:text>obra}eniq</xsl:text>
/POS45/AHT<xsl:text>na~ kon kol</xsl:text>
/CRLF
/CRLF
</xsl:template>
<xsl:template match="VV">
/POS00/AHT<xsl:value-of select="VN"/>
/POS04/AHT<xsl:value-of select="KV"/>
/POS07/AHT<xsl:value-of select="KL"/>
/POS11/AHT<xsl:value-of select="PT"/>
/POS17/AHT<xsl:value-of select="ROD"/>
/POS21/AHT<xsl:value-of select="VB"/>
/CRLF
/POS21/AHT<xsl:value-of select="W2"/>
/POS02/RVTU
/POS31/AHT<xsl:value-of select="VP/C1"/>
/CRLF
/POS31/AHT<xsl:value-of select="VP/C2"/>
/POS02/RVTU
/POS45/AHT<xsl:value-of select="HH"/>
/POS49/AHT<xsl:value-of select="HK"/>
/POS53/AHT<xsl:value-of select="SM"/>
<xsl:variable name="R1C" select="count(R/R1)"/>
/POS57/AHT<xsl:for-each select="R/R1">
<xsl:value-of select="."/>
<xsl:text> </xsl:text>
</xsl:for-each>
<xsl:if test="BAG">
<xsl:choose>
<xsl:when test="$R1C &gt;= 6">
/CRLF
/POS57/AHT<xsl:text>bag</xsl:text>
/CRLF
</xsl:when>
<xsl:otherwise>
<xsl:text>bag</xsl:text>
/CRLF
</xsl:otherwise>
</xsl:choose>
</xsl:if>
<xsl:if test="N3 and C6">
/POS57/AHT<xsl:text>k </xsl:text>
/POS59/AHT<xsl:value-of select="N3"/>
<xsl:text> </xsl:text>
<xsl:value-of select="C6"/>/CRLF
</xsl:if>
<xsl:if test="N2 and C5">
/POS57/AHT<xsl:text>ot </xsl:text>
/POS60/AHT<xsl:value-of select="N2"/>
<xsl:text> </xsl:text>
<xsl:value-of select="C5"/>/CRLF
</xsl:if>
<xsl:if test="TR">
/POS31/AHT<xsl:text>trafaret:</xsl:text>/CRLF
<xsl:for-each select="TR">
/POS31/AHT<xsl:value-of select="C"/>
/POS45/AHT<xsl:value-of select="HH"/>
/POS49/AHT<xsl:value-of select="HK"/>
/POS53/AHT<xsl:value-of select="SM"/>
/CRLF
</xsl:for-each>
</xsl:if>
</xsl:template>
<xsl:template mode="T8020" match="RESP">
<xsl:text disable-output-escaping="yes">/POS02/AVT</xsl:text>
<xsl:call-template name="ZAGOLOVOK"/>
<xsl:apply-templates select="VV"/>
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
<xsl:when test="RESP/@TYPE ='SPR_TSCHEME'">
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
