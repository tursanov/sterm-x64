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
<xsl:value-of select="substring(@TZ, 1, 8)"/>
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
<xsl:template name="VAL2RPOS">
   <xsl:param name="VAL"/>
   <xsl:param name="POS"/>
/POS
<xsl:call-template name="NUM2POS">
   <xsl:with-param name="NUM" select="$POS + 1 - string-length($VAL)"/>
</xsl:call-template>
/AHT
<xsl:value-of select="$VAL"/>
</xsl:template>
<xsl:template mode="T8020" match="RESP">
<xsl:apply-templates select="PP"/>
<xsl:call-template name="CENTER">
  <xsl:with-param name="W" select="80"/>
  <xsl:with-param name="V">
    <xsl:apply-templates select="ZG"/>
  </xsl:with-param>
</xsl:call-template>
/CRLF
<xsl:apply-templates mode="SPR_TROUTE" select="A">
<xsl:with-param name="N1" select="PP/N1I"/>
</xsl:apply-templates>
</xsl:template>
<xsl:template name="FMV">
  <xsl:param name="F"/>
  <xsl:param name="M"/>
<xsl:choose>
<xsl:when test="$F and $M">
<xsl:text>, firmennyj</xsl:text>
</xsl:when>
<xsl:when test="$F and not($M)">
<xsl:text>, firmennyj</xsl:text>
</xsl:when>
<xsl:when test="not($F) and $M">
<xsl:text/>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
</xsl:template>
<xsl:template name="TKTP">
<xsl:param name="KTP"/>
<xsl:choose>
<xsl:when test="$KTP = 'w-skor'">
<xsl:text>, wysokoskorostnoj</xsl:text>
</xsl:when>
<xsl:when test="$KTP = 'skrst'">
<xsl:text>, skorostnoj</xsl:text>
</xsl:when>
<xsl:when test="$KTP = 'sk'">
<xsl:text>, skoryj</xsl:text>
</xsl:when>
<xsl:when test="$KTP = 'pass'">
<xsl:text>, passavirskij</xsl:text>
</xsl:when>
</xsl:choose>
</xsl:template>
<xsl:template name="PART1">
<xsl:value-of select="N1"/>
<xsl:text> </xsl:text>
<xsl:value-of select="Z1/C1"/>
<xsl:text> - </xsl:text>
<xsl:value-of select="Z1/C2"/>
</xsl:template>
<xsl:template match="PP">
<xsl:call-template name="CENTER">
  <xsl:with-param name="W" select="80"/>
  <xsl:with-param name="V" select="'sprawo~naq informaciq o mar{rute poezda'"/>
</xsl:call-template>
/CRLF
<xsl:call-template name="CENTER">
   <xsl:with-param name="W" select="80"/>
   <xsl:with-param name="V">
     <xsl:call-template name="PART1"/>
     <xsl:call-template name="TKTP">
       <xsl:with-param name="KTP" select="KTP"/>
     </xsl:call-template>
     <xsl:call-template name="FMV">
       <xsl:with-param name="F" select="FIRM"/>
       <xsl:with-param name="M" select="MV"/>
     </xsl:call-template>
   </xsl:with-param>
</xsl:call-template>
/CRLF
<xsl:call-template name="CENTER">
   <xsl:with-param name="W" select="80"/>
   <xsl:with-param name="V">
     <xsl:text>data otprawleniq </xsl:text>
     <xsl:call-template name="YMD2DMY">
        <xsl:with-param name="YMD" select="D1"/>
     </xsl:call-template>
     <xsl:choose>
       <xsl:when test="LT">
         <xsl:text> mestnoe wremq</xsl:text>
       </xsl:when>
       <xsl:otherwise>
         <xsl:text> wremq moskowskoe</xsl:text>
       </xsl:otherwise>
     </xsl:choose>
   </xsl:with-param>
</xsl:call-template>
/CRLF
</xsl:template>
<xsl:template match="AP">
  <xsl:param name="POS"/>
<xsl:choose>
<xsl:when test="C1 and C2">
/POS
<xsl:value-of select="$POS"/>
/AHT
<xsl:text> </xsl:text>
<xsl:value-of select="C1"/>
<xsl:text> - </xsl:text>
<xsl:value-of select="C2"/>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
<xsl:choose>
<xsl:when test="count(A1) = 1">
<xsl:text> (nitka: </xsl:text>
<xsl:value-of select="A1[1]"/>
<xsl:text>)</xsl:text>
</xsl:when>
<xsl:when test="count(A1) &gt; 1">
<xsl:text> (nitki: </xsl:text>
<xsl:for-each select="A1">
<xsl:value-of select="."/>
</xsl:for-each>
<xsl:text>)</xsl:text>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
/CRLF
</xsl:template>
<xsl:template mode="SPR_TROUTE" match="A">
<xsl:param name="N1"/>
/CRLF
<xsl:value-of select="AA"/><xsl:text>:</xsl:text>
<xsl:apply-templates select="AP">
  <xsl:with-param name="POS">
    <xsl:call-template name="NUM2POS">
      <xsl:with-param name="NUM" select="string-length(AA) + 1"/>
    </xsl:call-template>
  </xsl:with-param>
</xsl:apply-templates>
/CRLF
<xsl:text>
/POS00/AHTstanciq
/POS13/AHTpribytie
/POS25/AHTstoqnka
/POS35/AHTotprawlenie
/POS47/AHTkol-wo
/POS54/AHTrasstoq-
/POS63/AHTuro-
/POS68/AHTpriznaki
/CRLF
/POS47/AHTsutok
/POS54/AHTnie, km
/POS63/AHTwenx
/CRLF
</xsl:text>
<xsl:apply-templates select="AC">
<xsl:with-param name="N1" select="$N1"/>
</xsl:apply-templates>
</xsl:template>
<xsl:template match="DLT">
<xsl:choose>
<xsl:when test="not (.)">
</xsl:when>
<xsl:when test="string(number(.)) = 'NaN'">
</xsl:when>
<xsl:when test=". &gt; 0">
<xsl:text> msk+</xsl:text>
<xsl:value-of select="number(.)"/>
</xsl:when>
<xsl:when test=". &lt; 0">
<xsl:text> msk</xsl:text>
<xsl:value-of select="."/>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
</xsl:template>
<xsl:template match="AC">
<xsl:param name="N1"/>
<xsl:choose>
<xsl:when test="TS and (D8 = D7) and (T6=T7)">
</xsl:when>
<xsl:otherwise>
<xsl:if test="C">
/POS00/AHT
<xsl:value-of select="C"/>
/CRLF
</xsl:if>
/POS00/AHT
<xsl:if test="NPRHD">
<xsl:text>*(</xsl:text><xsl:value-of select="NPRHD"/><xsl:text>) </xsl:text>
</xsl:if>
<xsl:value-of select="CC"/>
<xsl:if test="C">
/POS02/RVTU
</xsl:if>
<xsl:if test="D8 and T6">
/POS13/AHT
<xsl:call-template name="YMD2DMY">
   <xsl:with-param name="YMD" select="D8"/>
</xsl:call-template>
/CRLF
/POS13/AHT
<xsl:value-of select="substring(T6, 1, 5)"/>
<xsl:apply-templates select="DLT"/>
/POS02/RVTU
</xsl:if>
<xsl:if test="T2">
<xsl:choose>
<xsl:when test="substring(T2, 1, 2) = '00' ">
<xsl:call-template name="VAL2RPOS">
   <xsl:with-param name="VAL" select="number(substring(T2, 4, 2))"/>
   <xsl:with-param name="POS" select="30"/>
</xsl:call-template>
<xsl:text> min</xsl:text>
</xsl:when>
<xsl:otherwise>
<xsl:call-template name="VAL2RPOS">
   <xsl:with-param name="VAL" select="number(substring(T2, 1, 2))"/>
   <xsl:with-param name="POS" select="25"/>
</xsl:call-template>
<xsl:text> ~</xsl:text>
<xsl:call-template name="VAL2RPOS">
   <xsl:with-param name="VAL" select="number(substring(T2, 4, 2))"/>
   <xsl:with-param name="POS" select="30"/>
</xsl:call-template>
<xsl:text> min</xsl:text>
</xsl:otherwise>
</xsl:choose>
</xsl:if>
<xsl:if test="D7 and T7">
/POS36/AHT
<xsl:call-template name="YMD2DMY">
   <xsl:with-param name="YMD" select="D7"/>
</xsl:call-template>
/CRLF
/POS36/AHT
<xsl:value-of select="substring(T7, 1, 5)"/>
<xsl:apply-templates select="DLT"/>
/POS02/RVTU
</xsl:if>
<xsl:if test="SU">
/POS49/AHT
<xsl:value-of select="SU"/>
</xsl:if>
<xsl:call-template name="VAL2RPOS">
   <xsl:with-param name="VAL" select="L"/>
   <xsl:with-param name="POS" select="60"/>
</xsl:call-template>
<xsl:call-template name="VAL2RPOS">
   <xsl:with-param name="VAL" select="US"/>
   <xsl:with-param name="POS" select="65"/>
</xsl:call-template>
<xsl:choose>
<xsl:when test="TS or X or (I and $N1) or TG">
<xsl:if test="TS">
/POS68/AHT<xsl:text>tehni~eskaq</xsl:text>/CRLF
/POS68/AHT<xsl:text>stoqnka</xsl:text>/CRLF
</xsl:if>
<xsl:if test="X">
/POS68/AHT<xsl:text>hwostom</xsl:text>/CRLF
/POS68/AHT<xsl:text>wpered</xsl:text>/CRLF
</xsl:if>
<xsl:if test="I and $N1">
/POS68/AHT<xsl:text>otprawlqetsq</xsl:text>/CRLF
/POS68/AHT<xsl:text>poezdom </xsl:text><xsl:value-of select="$N1"/>/CRLF
</xsl:if>
<xsl:if test="TG">
/POS68/AHT<xsl:text>tarifnaq</xsl:text>/CRLF
/POS68/AHT<xsl:text>granica</xsl:text>/CRLF
</xsl:if>
</xsl:when>
<xsl:otherwise>
/CRLF
/CRLF
</xsl:otherwise>
</xsl:choose>
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
<xsl:when test="RESP/@TYPE ='SPR_TROUTE'">
<xsl:choose>
<xsl:when test="@W='80' and @H='20' and substring(@T , 1 , 1)='S'">
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
