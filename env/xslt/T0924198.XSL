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
<xsl:template name="DT">
<xsl:param name="NP"/>
<xsl:param name="FR"/>
<xsl:param name="D"/>
<xsl:param name="T"/>
<xsl:param name="NAME"/>
<xsl:param name="CODE"/>
/POS27/AHT<xsl:text>otprawlenie wremq mestnoe </xsl:text>/CRLF
/POS00/AHT
<xsl:text>poezdom </xsl:text>
<xsl:value-of select="$NP"/>
<xsl:if test="$FR">
/CRLF<xsl:text>firm</xsl:text>/POS02/RVTU
</xsl:if>
/POS19/AHT
<xsl:value-of select="substring($D, 9, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring($D, 6, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring($D, 1, 4)"/>
<xsl:text> w </xsl:text>
<xsl:value-of select="substring($T, 1, 5)"/>
/POS43/AHT
<xsl:value-of select="$NAME"/>
<xsl:text> (</xsl:text>
<xsl:value-of select="$CODE"/>
<xsl:text>)</xsl:text>/CRLF
</xsl:template>
<xsl:template name="DT2">
<xsl:param name="D2"/>
<xsl:param name="T2"/>
<xsl:param name="NUMT"/>
<xsl:param name="NM"/>
<xsl:param name="TFF"/>
<xsl:param name="TFF3"/>
<xsl:param name="RAS4"/>
<xsl:param name="INT"/>
/POS00/AHT<xsl:text>prodan </xsl:text>
<xsl:value-of select="substring($D2, 9, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring($D2, 6, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring($D2, 1, 4)"/>
<xsl:text> w </xsl:text>
<xsl:value-of select="substring($T2, 1, 5)"/>
<xsl:text>, </xsl:text>
<xsl:value-of select="$NUMT"/>
<xsl:text>, </xsl:text>
<xsl:value-of select="$NM"/>/CRLF
<xsl:text>stoimostx </xsl:text>
<xsl:value-of select="$TFF"/>
<xsl:if test="$TFF3">
<xsl:text>/</xsl:text>
<xsl:value-of select="$TFF3"/>
</xsl:if>
<xsl:if test="SOC">
<xsl:text> obxqwl.cennostx </xsl:text>
<xsl:value-of select="SOC"/>
</xsl:if>
<xsl:text> wid ras~eta </xsl:text>
<xsl:choose>
<xsl:when test="$RAS4 = '|f'">
<xsl:text>bankowskaq karta</xsl:text>
</xsl:when>
<xsl:when test="$RAS4 = '|r'">
<xsl:text>bankowskaq karta</xsl:text>
</xsl:when>
<xsl:when test="$RAS4 = 'p`'">
<xsl:text>beznali~nyj</xsl:text>
</xsl:when>
<xsl:when test="$RAS4 = 'W'">
<xsl:text>bankowskaq karta</xsl:text>
</xsl:when>
<xsl:when test="$RAS4 = 'Wn'">
<xsl:text>nali~nyj</xsl:text>
</xsl:when>
<xsl:when test="$RAS4 = 'n'">
<xsl:text>nali~nyj</xsl:text>
</xsl:when>
<xsl:otherwise>
???
</xsl:otherwise>
</xsl:choose>
<xsl:choose>
<xsl:when test="$RAS4">
<xsl:text> (</xsl:text>
<xsl:value-of select="$RAS4"/>
<xsl:text>)</xsl:text>
</xsl:when>
<xsl:otherwise>
net wida ras~eta
</xsl:otherwise>
</xsl:choose>
<xsl:if test="$INT">
<xsl:text>, integrator</xsl:text>
</xsl:if>
/CRLF
</xsl:template>
<xsl:template name="DT3">
<xsl:param name="NP3"/>
<xsl:param name="D3"/>
<xsl:param name="T3"/>
<xsl:param name="NAME3"/>
<xsl:param name="CODE3"/>
/POS28/AHT<xsl:text>pribytie</xsl:text><xsl:if test="$D3"><xsl:text> wremq mestnoe</xsl:text></xsl:if>/CRLF
<xsl:if test="$NP3">/POS00/AHT<xsl:text>poezdom </xsl:text><xsl:value-of select="$NP3"/></xsl:if>
<xsl:if test="$D3">/POS19/AHT<xsl:value-of select="substring($D3, 9, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring($D3, 6, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring($D3, 1, 4)"/></xsl:if>
<xsl:if test="$T3"><xsl:text> w </xsl:text>
<xsl:value-of select="substring($T3, 1, 5)"/></xsl:if>
/POS43/AHT<xsl:value-of select="$NAME3"/><xsl:text> (</xsl:text><xsl:value-of select="$CODE3"/><xsl:text>)</xsl:text>
/CRLF
/CRLF
</xsl:template>
<xsl:template name="TOST">
<xsl:param name="VAL"/><xsl:value-of select="substring($VAL, string-length($VAL)-1, 2)"/>
</xsl:template>
<xsl:template mode="T8020" match="RESP">
/POS19/AHT<xsl:text>sprawo~naq informaciq ob okazywaemyh passaviru uslugah</xsl:text>/CRLF
<xsl:choose>
<xsl:when test="PRZD">
<xsl:apply-templates select="PRZD">
<xsl:with-param name="OO1" select="OO"/>
</xsl:apply-templates>
</xsl:when>
<xsl:when test="PRV">
<xsl:apply-templates select="PRV">
</xsl:apply-templates>
</xsl:when>
<xsl:when test="DPITN">
<xsl:apply-templates select="DPITN">
</xsl:apply-templates>
</xsl:when>
<xsl:when test="SERVICE[KKAT='01']">
<xsl:apply-templates mode="KKAT01" select="SERVICE[KKAT='01']">
</xsl:apply-templates>
</xsl:when>
<xsl:when test="SERVICE[KKAT='02']">
<xsl:apply-templates mode="KKAT02" select="SERVICE[KKAT='02']">
</xsl:apply-templates>
</xsl:when>
<xsl:otherwise>
<xsl:text>otsutstwuet informaciq o proezdnyh, perewozo~nyh dokumentah,</xsl:text>/CRLF
<xsl:text>o dokumentah o dopolnitelxnom pitanii i dopolnitelxnyh uslugah</xsl:text>/CRLF
</xsl:otherwise>
</xsl:choose>
</xsl:template>
<xsl:template match="PRZD">
<xsl:param name="OO1"/>
<xsl:choose>
<xsl:when test="PNR">
/POS34/AHT
<xsl:text>PNR </xsl:text>
<xsl:value-of select="PNR"/>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>/CRLF
/POS00/AHT<xsl:text>proezdnoj dokument </xsl:text>
<xsl:choose>
<xsl:when test="NB">
<xsl:value-of select="NB"/>
</xsl:when>
<xsl:when test="SN1">
<xsl:value-of select="SN1"/>
</xsl:when>
<xsl:otherwise>
<xsl:text>net nomera dokumenta</xsl:text>
</xsl:otherwise>
</xsl:choose>
<xsl:if test="SVD">
<xsl:text> k dokumentu </xsl:text>
<xsl:value-of select="SVD"/>
</xsl:if>
/CRLF
<xsl:call-template name="DT2">
<xsl:with-param name="D2" select="SD"/>
<xsl:with-param name="T2" select="TB"/>
<xsl:with-param name="NUMT" select="KN"/>
<xsl:with-param name="NM" select="NAME_KP"/>
<xsl:with-param name="TFF" select="TF"/>
<xsl:with-param name="TFF3" select="TF3"/>
<xsl:with-param name="RAS4" select="VR"/>
<xsl:with-param name="INT" select="IG"/>
</xsl:call-template>
<xsl:call-template name="DT">
<xsl:with-param name="NP" select="DEP/N1"/>
<xsl:with-param name="FR" select="DEP/FIRM"/>
<xsl:with-param name="D" select="DEP/LD3"/>
<xsl:with-param name="T" select="DEP/LT1"/>
<xsl:with-param name="NAME" select="DEP/C1"/>
<xsl:with-param name="CODE" select="DEP/CC1"/>
</xsl:call-template>
<xsl:call-template name="DT3">
<xsl:with-param name="NP3" select="ARR/N5"/>
<xsl:with-param name="D3" select="ARR/LD6"/>
<xsl:with-param name="T3" select="ARR/LT4"/>
<xsl:with-param name="NAME3" select="ARR/C2"/>
<xsl:with-param name="CODE3" select="ARR/CC2"/>
</xsl:call-template>
<xsl:text>wagon </xsl:text><xsl:value-of select="CAR/VN"/><xsl:text> </xsl:text>
<xsl:value-of select="CAR/KV"/>
<xsl:text>/</xsl:text>
<xsl:value-of select="CAR/KL"/>
<xsl:text>, </xsl:text>
<xsl:value-of select="CARRIER/VB"/>
<xsl:text>, </xsl:text>
<xsl:choose>
<xsl:when test="IP/WC">
<xsl:text>kupe </xsl:text>
<xsl:choose>
<xsl:when test="IP/WC = 'ven'">
<xsl:text>venskoe </xsl:text>
</xsl:when>
<xsl:when test="IP/WC = 'muv'">
<xsl:text>muvskoe </xsl:text>
</xsl:when>
</xsl:choose>
</xsl:when>
</xsl:choose>
<xsl:choose>
<xsl:when test="R1"><xsl:text>serwis: </xsl:text><xsl:value-of select="R1"/>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
/CRLF
<xsl:if test="DPIT">
<xsl:text>wozmovno dop pitanie</xsl:text>/CRLF
</xsl:if>
<xsl:if test="VPIT">
<xsl:text>wozmoven wybor pitaniq</xsl:text>/CRLF
</xsl:if>
<xsl:if test="GRP">
<xsl:text>gruppowoj nomera mest: </xsl:text>
<xsl:for-each select="NN/N1">
<xsl:value-of select="."/>
<xsl:if test="position() != last()">
<xsl:text>,</xsl:text>
</xsl:if>
</xsl:for-each>
</xsl:if>
<xsl:if test="GRP">
/CRLF
</xsl:if>
<xsl:choose>
<xsl:when test="CARRIER/VB = ('fpk' or 'grand' or 'grandt') and KV = ('m' or 'l' or 'k') and not(Y4) and not(YD)">
<xsl:text>prowoz viwotnyh razre{en pri wykupe wsego kupe</xsl:text>
</xsl:when>
<xsl:when test="CARRIER/VB = ('fpk' or 'grand' or 'grandt') and KV = ('p' or 's' or 'o') and not(Y4) and not(YD)">
<xsl:text>prowoz viwotnyh zapre}en</xsl:text>
</xsl:when>
<xsl:when test="CARRIER/VB = ('fpk' or 'grand' or 'grandt') and not(Y4) and YD">
<xsl:text>prowoz viwotnyh zapre}en</xsl:text>
</xsl:when>
<xsl:when test="CARRIER/VB = ('fpk' or 'grand' or 'grandt') and Y4 and not(YD)">
<xsl:text>prowoz viwotnyh razre{en</xsl:text>
</xsl:when>
<xsl:when test="CARRIER/VB = ('fpk' or 'grand' or 'grandt') and Y4 and YD">
<xsl:text>prawila prowoza viwotnyh uto~nqjte u perewoz~ika</xsl:text>
</xsl:when>
<xsl:when test="CARRIER/VB != ('fpk' or 'grand' or 'grandt') and not(Y4) and not(YD)">
<xsl:text>prawila prowoza viwotnyh uto~nqjte u perewoz~ika</xsl:text>
</xsl:when>
<xsl:when test="CARRIER/VB != ('fpk' or 'grand' or 'grandt') and not(Y4) and YD">
<xsl:text>prowoz viwotnyh zapre}en</xsl:text>
</xsl:when>
<xsl:when test="CARRIER/VB != ('fpk' or 'grand' or 'grandt') and Y4 and not(YD)">
<xsl:text>prowoz viwotnyh razre{en</xsl:text>
</xsl:when>
<xsl:otherwise>
<xsl:text>prawila prowoza viwotnyh uto~nqjte u perewoz~ika</xsl:text>
</xsl:otherwise>
</xsl:choose>
/CRLF
<xsl:choose>
<xsl:when test="YO and YL">
<xsl:text>numeraciq wagonow s hwosta poezda </xsl:text>
<xsl:text>numeraciq wagonow s golowy poezda </xsl:text>
</xsl:when>
<xsl:when test="YO">
<xsl:text>numeraciq wagonow s hwosta poezda </xsl:text>
</xsl:when>
<xsl:when test="YL">
<xsl:text>numeraciq wagonow s golowy poezda </xsl:text>
</xsl:when>
</xsl:choose>
/CRLF
/POS00/AHT<xsl:text>mesta</xsl:text>
/POS31/AHT<xsl:text>passavir</xsl:text>
/POS64/AHT<xsl:text>status</xsl:text>/CRLF
<xsl:for-each select="IP/PI">
<xsl:choose>
<xsl:when test="N1">
/POS00/AHT
<xsl:for-each select="N1">
<xsl:value-of select="."/>
</xsl:for-each>
</xsl:when>
<xsl:otherwise>
<xsl:text>net mest</xsl:text>
</xsl:otherwise>
</xsl:choose>
<xsl:choose>
<xsl:when test="PS or IZ or GD or GR or FM or TEL or MAIL or DM">
/POS10/AHT
<xsl:variable name="nomer" select="position()"/>
/POS10/AHT<xsl:value-of select="$nomer"/><xsl:text>. </xsl:text>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
<xsl:choose>
<xsl:when test="PS">
<xsl:value-of select="PS"/>
<xsl:text>/</xsl:text>
/CRLF
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
<xsl:choose>
<xsl:when test="IZ">
/POS13/AHT
<xsl:value-of select="substring-before(IZ, '=')"/>
/CRLF
/POS13/AHT<xsl:value-of select="substring-after(IZ, '=')"/>
/CRLF
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
<xsl:choose>
<xsl:when test="GD or GR or FM">
<xsl:if test="GD">
/POS13/AHT
<xsl:value-of select="GD"/>
</xsl:if>
<xsl:if test="GR">
<xsl:text>/</xsl:text>
<xsl:value-of select="GR"/>
</xsl:if>
<xsl:if test="FM">
<xsl:text>/</xsl:text>
<xsl:value-of select="FM"/>
/CRLF
</xsl:if>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
<xsl:choose>
<xsl:when test="TEL or MAIL or MD">
/POS13/AHT
<xsl:choose>
<xsl:when test="TEL">
<xsl:value-of select="TEL"/>
<xsl:if test="not(MAIL or MD)">
/CRLF
</xsl:if>
<xsl:if test="MAIL or MD">
<xsl:text> </xsl:text>
</xsl:if>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
<xsl:choose>
<xsl:when test="MAIL">
<xsl:value-of select="MAIL"/>
<xsl:if test="not(MD)">
/CRLF
</xsl:if>
<xsl:if test="MD">
<xsl:text> </xsl:text>
</xsl:if>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
<xsl:choose>
<xsl:when test="MD">
<xsl:text>medrabotnik</xsl:text>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
<xsl:if test="PS">
/POS02/RVTU
</xsl:if>
<xsl:if test="IZ">
/POS04/RVTU
</xsl:if>
<xsl:if test="GD or GR or FM">
/POS02/RVTU
</xsl:if>
</xsl:for-each>
<xsl:call-template name="STATUS">
<xsl:with-param name="K" select="K1"/>
<xsl:with-param name="OO1" select="OO"/>
<xsl:with-param name="DETS" select="DET"/>
<xsl:with-param name="GRUP" select="GRU"/>
<xsl:with-param name="ECP" select="EC"/>
<xsl:with-param name="POEZDA" select="POEZD"/>
<xsl:with-param name="HEIGHT">
<xsl:choose>
<xsl:when test="IP/PI/TEL or IP/PI/MAIL or IP/PI/MD">
5
</xsl:when>
<xsl:otherwise>
4
</xsl:otherwise>
</xsl:choose>
</xsl:with-param>
</xsl:call-template>
<xsl:choose>
<xsl:when test="IP/PI/PS or IP/PI/IZ or IP/PI/GD or IP/PI/GR or IP/PI/FM">
</xsl:when>
<xsl:otherwise>
/POS06/RVTU
</xsl:otherwise>
</xsl:choose>
/POS00/AHT
<xsl:choose>
<xsl:when test="IP/D5 and IP/PI2/*">
<xsl:text>bez mesta</xsl:text>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>
/POS10/AHT
<xsl:choose>
<xsl:when test="IP/D5">
<xsl:if test="IP/PI2/PS">
<xsl:text>2. </xsl:text>
<xsl:value-of select="IP/PI2/PS"/>
<xsl:text>/</xsl:text>
</xsl:if>
</xsl:when>
</xsl:choose>
/CRLF
/POS13/AHT<xsl:value-of select="substring-before(IP/PI2/IZ, '=')"/>/CRLF
/POS13/AHT<xsl:value-of select="substring-after(IP/PI2/IZ, '=')"/>/CRLF
/POS13/AHT<xsl:if test="IP/PI2/GD">
<xsl:value-of select="IP/PI2/GD"/>
</xsl:if>
<xsl:if test="IP/PI2/GR">
<xsl:text>/</xsl:text>
</xsl:if>
<xsl:if test="IP/PI2/GR">
<xsl:value-of select="IP/PI2/GR"/>
</xsl:if>
<xsl:if test="IP/PI2/FM">
<xsl:text>/</xsl:text>
<xsl:value-of select="IP/PI2/FM"/>
</xsl:if>
/CRLF
<xsl:if test="not(IP/PI2/PS and IP/PI2/IZ and IP/PI2/GD and IP/PI2/GR and IP/PI2/FM)">
/POS04/RVTU
</xsl:if>
<xsl:if test="not(IP/D5 and IP/PI2/*)">
/POS02/RVTU
</xsl:if>
<xsl:text>wid dokumenta: </xsl:text>
<xsl:choose>
<xsl:when test="VD">
<xsl:value-of select="VD"/>
</xsl:when>
<xsl:when test="SHL/SL">
<xsl:choose>
<xsl:when test="SHL/W1">
<xsl:value-of select="concat(SHL/SL, '/', SHL/W1)"/>
</xsl:when>
<xsl:otherwise>
<xsl:value-of select="SHL/SL"/>
</xsl:otherwise>
</xsl:choose>
</xsl:when>
<xsl:otherwise>
<xsl:value-of select="SHL/SL"/>
</xsl:otherwise>
</xsl:choose>
<xsl:choose>
<xsl:when test="IP/DL">
<xsl:text>, osnowanie: </xsl:text>
<xsl:value-of select="IP/DL"/>
<xsl:text/>
</xsl:when>
<xsl:otherwise>
</xsl:otherwise>
</xsl:choose>/CRLF
<xsl:choose>
<xsl:when test="NVZ">
<xsl:text>newozwratnyj</xsl:text>
<xsl:if test="NVZ and (IP/PI/BON or INV or GRP or IP/PI/UP or IP/PI/RZ)">, </xsl:if>
</xsl:when>
</xsl:choose>
<xsl:choose>
  <xsl:when test="IP/PI/BON">
    <xsl:text>karta: </xsl:text><xsl:value-of select="IP/PI/BON"/>
    <xsl:if test="IP/PI/BON and (INV or GRP or IP/PI/UP or IP/PI/RZ)">, </xsl:if>
  </xsl:when>
  <xsl:otherwise>
  </xsl:otherwise>
</xsl:choose>
<xsl:choose>
  <xsl:when test="INV">
  <xsl:text>malomobilxnyj</xsl:text>
  <xsl:if test="INV and (GRP or IP/PI/UP or IP/PI/RZ)">, </xsl:if>
</xsl:when>
</xsl:choose>
<xsl:choose>
  <xsl:when test="GRP">
  <xsl:text>gruppowoj</xsl:text>
  <xsl:if test="GRP and (IP/PI/UP or IP/PI/RZ)">, </xsl:if>
</xsl:when>
</xsl:choose>
<xsl:choose>
  <xsl:when test="IP/PI/UP">
  <xsl:text>updvd</xsl:text>
  <xsl:if test="IP/PI/UP and IP/PI/RZ">, </xsl:if>
</xsl:when>
</xsl:choose>
<xsl:if test="IP/PI/RZ">
  <xsl:text>razr</xsl:text>
</xsl:if>
/CRLF
<xsl:if test="PRM">
<xsl:text>sme{annaq perewozka </xsl:text>
</xsl:if>
<xsl:if test="ABT">
<xsl:text>mulxtimodalxnaq perewozka </xsl:text>
</xsl:if>
<xsl:if test="MS">
<xsl:text>wodnyj transport </xsl:text>
</xsl:if>
<xsl:if test="PRM or ABT or MS">
/CRLF
</xsl:if>
<xsl:if test="DM">
<xsl:text>wozwrat wozmoven pri predxqwlenii wseh dokumentow zakaza</xsl:text>
/CRLF
</xsl:if>
<xsl:apply-templates select="PRV"/>
/CRLF
<xsl:apply-templates select="DPITN"/>
<xsl:if test="SERVICE[KKAT='01']">
<xsl:apply-templates mode="KKAT01" select="SERVICE[KKAT='01']">
</xsl:apply-templates>
</xsl:if>
<xsl:if test="SERVICE[KKAT='02']">
<xsl:apply-templates mode="KKAT02" select="SERVICE[KKAT='02']">
</xsl:apply-templates>
</xsl:if>
</xsl:template>
<xsl:template match="PRV">
<xsl:text>perewozo~nyj dokument: </xsl:text>
<xsl:value-of select="EPD"/>/CRLF
<xsl:call-template name="DT2">
<xsl:with-param name="D2" select="SD"/>
<xsl:with-param name="T2" select="TB"/>
<xsl:with-param name="NUMT" select="KN"/>
<xsl:with-param name="TFF" select="TF"/>
<xsl:with-param name="RAS4" select="VR"/>
<xsl:with-param name="INT" select="IG"/>
</xsl:call-template>
<xsl:call-template name="DT">
<xsl:with-param name="NP" select="DEP/N1"/>
<xsl:with-param name="D" select="DEP/LD3"/>
<xsl:with-param name="T" select="DEP/LT1"/>
<xsl:with-param name="NAME" select="DEP/C1"/>
<xsl:with-param name="CODE" select="DEP/CC1"/>
</xsl:call-template>
<xsl:call-template name="DT3">
<xsl:with-param name="NP3" select="ARR/N5"/>
<xsl:with-param name="D3" select="ARR/LD6"/>
<xsl:with-param name="T3" select="ARR/LT4"/>
<xsl:with-param name="NAME3" select="ARR/C2"/>
<xsl:with-param name="CODE3" select="ARR/CC2"/>
</xsl:call-template>
<xsl:text>wagon </xsl:text>
<xsl:value-of select="CAR/VN"/>
<xsl:text>, wagon passavira </xsl:text>
<xsl:value-of select="CAR/VNP"/>
<xsl:text>, </xsl:text>
<xsl:value-of select="CARRIER/VB"/>/CRLF
/POS00/AHT<xsl:text>info</xsl:text>
/POS31/AHT<xsl:text>otprawitelx</xsl:text>
/POS64/AHT<xsl:text>status</xsl:text>/CRLF
/POS00/AHT<xsl:text>wid otprawki: </xsl:text>
<xsl:choose>
<xsl:when test="not (VO)">
<xsl:text>net statusa</xsl:text>/CRLF
<xsl:text>bagava</xsl:text>
</xsl:when>
<xsl:when test="VO = 'b'">
<xsl:text>bagav</xsl:text>
</xsl:when>
<xsl:when test="VO = 'gn'">
<xsl:text>gruzobagav</xsl:text>/CRLF
<xsl:text>ot naseleniq</xsl:text>
</xsl:when>
<xsl:when test="VO = 'gp'">
<xsl:text>gruzobagav</xsl:text>/CRLF
<xsl:text>ot predpriqtiq</xsl:text>
</xsl:when>
</xsl:choose>/CRLF
/POS00/AHT<xsl:text>wid bagava: </xsl:text>
<xsl:choose>
<xsl:when test="VBG = 'r'">
<xsl:text>ru~naq kladx</xsl:text>
</xsl:when>
<xsl:when test="VBG = 'u'">
<xsl:text>ru~naq kladx na osobyh uslowiqh</xsl:text>
</xsl:when>
<xsl:when test="VBG = 'v'">
<xsl:text>viwotnye na rukah</xsl:text>
</xsl:when>
<xsl:when test="VBG = 'k'">
<xsl:text>bagav w kupe</xsl:text>
</xsl:when>
<xsl:when test="VBG = 'p'">
<xsl:text>awtomobilx</xsl:text>
</xsl:when>
<xsl:when test="VBG = 'pb'">
<xsl:text>awtomobilx bez mesta</xsl:text>
</xsl:when>
<xsl:when test="VBG = 'm'">
<xsl:text>mototehnika</xsl:text>
</xsl:when>
<xsl:when test="VBG = 'mb'">
<xsl:text>mototehnika bez mesta</xsl:text>
</xsl:when>
<xsl:when test="VBG = 'vk'">
<xsl:text>viwotnye w kupe</xsl:text>
</xsl:when>
<xsl:when test="VBG = 'vb'">
<xsl:text>viwotnye w kupe bez mesta</xsl:text>
</xsl:when>
<xsl:when test="VBG = '~'">
<xsl:text>malogabaritnaq otprawka na rukah</xsl:text>
</xsl:when>
<xsl:when test="VBG = 'kb'">
<xsl:text>w kupe bez mesta</xsl:text>
</xsl:when>
</xsl:choose>
<xsl:choose>
<xsl:when test="KKAT">/CRLF
<xsl:text>kategoriq gruza: </xsl:text>
<xsl:value-of select="KKAT"/>
</xsl:when>
</xsl:choose>
<xsl:choose>
<xsl:when test="TWYX">/CRLF
<xsl:text>naimenowanie gruza: </xsl:text>
<xsl:value-of select="TWYX"/>
</xsl:when>
</xsl:choose>
/CRLF
/POS00/AHT<xsl:text>wes: </xsl:text>
<xsl:value-of select="VES"/>/CRLF
/POS00/AHT<xsl:text>kol-wo mest: </xsl:text>
<xsl:value-of select="MST"/>
/POS06/RVTU
<xsl:if test="KKAT">
/POS02/RVTU
</xsl:if>
<xsl:if test="TWYX">
/POS02/RVTU
</xsl:if>
<xsl:variable name="iz" select="IZ"/>
<xsl:choose>
<xsl:when test="string-length($iz) &gt; 36">
/POS27/AHT<xsl:value-of select="substring($iz, 1, 36)"/>
/CRLF
/POS27/AHT<xsl:value-of select="substring($iz, 37)"/>
/POS02/RVTU
</xsl:when>
<xsl:otherwise>
/POS27/AHT<xsl:value-of select="$iz"/>/CRLF
/POS02/RVTU
</xsl:otherwise>
</xsl:choose>
<xsl:call-template name="STATUS">
<xsl:with-param name="K" select="K1"/>
<xsl:with-param name="OO1" select="OO"/>
<xsl:with-param name="DETS" select="DET"/>
<xsl:with-param name="GRUP" select="GRU"/>
<xsl:with-param name="ECP" select="EC"/>
<xsl:with-param name="POEZDA" select="POEZD"/>
<xsl:with-param name="HEIGHT" select="3"/>
</xsl:call-template>
<xsl:if test="KKAT">
/CRLF
</xsl:if>
<xsl:if test="TWYX">
/CRLF
</xsl:if>
</xsl:template>
<xsl:template match="DPITN">
<xsl:text>dopolnitelxnoe pitanie: </xsl:text>
<xsl:value-of select="KRS"/>/CRLF
<xsl:call-template name="DT2">
<xsl:with-param name="D2" select="SD"/>
<xsl:with-param name="T2" select="TB"/>
<xsl:with-param name="NUMT" select="KN"/>
<xsl:with-param name="RAS4" select="VR"/>
<xsl:with-param name="INT" select="IG"/>
</xsl:call-template>
<xsl:text>zawtraki: </xsl:text>
<xsl:value-of select="KOMP/@KZ"/>
<xsl:text>, </xsl:text>
<xsl:text>obedy: </xsl:text>
<xsl:value-of select="KOMP/@KO"/>
<xsl:text>, </xsl:text>
<xsl:text>uviny: </xsl:text>
<xsl:value-of select="KOMP/@KU"/>
/CRLF
/POS00/AHT<xsl:text>mesta</xsl:text>
/POS11/AHT<xsl:text>u~astok predostawleniq pitaniq</xsl:text>
/POS43/AHT<xsl:text>tip pitaniq</xsl:text>
/POS56/AHT<xsl:text>racion</xsl:text>
/POS64/AHT<xsl:text>status</xsl:text>/CRLF
<xsl:choose>
<xsl:when test="NN/N1">
<xsl:for-each select="NN/N1">
<xsl:choose>
<xsl:when test="(position() mod 2)=1">
/POS00/AHT<xsl:value-of select="."/>
<xsl:choose>
<xsl:when test="position() != last()"><xsl:text>,</xsl:text></xsl:when>
<xsl:otherwise>
/CRLF
</xsl:otherwise>
</xsl:choose>
</xsl:when>
<xsl:otherwise>
/POS05/AHT<xsl:value-of select="."/>
<xsl:if test="position() != last()"><xsl:text>,</xsl:text></xsl:if>/CRLF
</xsl:otherwise>
</xsl:choose>
</xsl:for-each>
/POS
<xsl:call-template name="TOST">
<xsl:with-param name="VAL" select="concat('0', floor((count(NN/N1)+1) div 2)*2)"/>
</xsl:call-template>
/RVTU
</xsl:when>
<xsl:otherwise>
<xsl:text>net mest</xsl:text>
</xsl:otherwise>
</xsl:choose>
<xsl:variable name="CDN1" select="floor((count(NN/N1)+1) div 2)"/>
<xsl:for-each select="KOMP/U">/POS11/AHT
<xsl:value-of select="concat(U1/C, '-', U2/CC)"/>
/POS46/AHT
<xsl:choose>
<xsl:when test="PIT = 'z'">
<xsl:text>zawtrak</xsl:text>
</xsl:when>
<xsl:when test="PIT = 'o'">
<xsl:text>obed</xsl:text>
</xsl:when>
<xsl:when test="PIT = 'u'">
<xsl:text>uvin</xsl:text>
</xsl:when>
</xsl:choose>
/POS59/AHT
<xsl:value-of select="VP"/>
/CRLF
</xsl:for-each>
/POS
<xsl:call-template name="TOST">
<xsl:with-param name="VAL" select="concat('0', count(KOMP/U)*2)"/>
</xsl:call-template>
/RVTU
<xsl:variable name="CDU" select="count(KOMP/U)"/>
<xsl:call-template name="STATUS">
<xsl:with-param name="K" select="K1"/>
<xsl:with-param name="OO1" select="OO"/>
<xsl:with-param name="DETS" select="DET"/>
<xsl:with-param name="GRUP" select="GRU"/>
<xsl:with-param name="ECP" select="EC"/>
<xsl:with-param name="POEZDA" select="POEZD"/>
<xsl:with-param name="HEIGHT">
<xsl:choose>
<xsl:when test="$CDN1 &gt; $CDU">
<xsl:value-of select="$CDN1"/>
</xsl:when>
<xsl:otherwise>
<xsl:value-of select="$CDU"/>
</xsl:otherwise>
</xsl:choose>
</xsl:with-param>
</xsl:call-template>
</xsl:template>
<xsl:template mode="KKAT01" match="SERVICE[KKAT='01']">
<xsl:text>dopolnitelxnaq usluga </xsl:text>
<xsl:value-of select="KRS"/>/CRLF
<xsl:choose>
<xsl:when test="K1 = 0"><xsl:text>oformlena </xsl:text></xsl:when>
<xsl:when test="K1 = 3"><xsl:text>annulirowan </xsl:text></xsl:when>
<xsl:when test="K1 = 4"><xsl:text>wozwra}en </xsl:text></xsl:when>
<xsl:otherwise>
<xsl:text>neizwestnoe zna~enie K1</xsl:text>
</xsl:otherwise>
</xsl:choose>
<xsl:value-of select="substring(SD, 9, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring(SD, 6, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring(SD, 1, 4)"/>
<xsl:text> w </xsl:text>
<xsl:value-of select="substring(TB, 1, 5)"/>
<xsl:text>, </xsl:text>
<xsl:value-of select="KN"/>
<xsl:text>, wid ras~eta </xsl:text>
<xsl:choose>
<xsl:when test="VR = '|f'">
<xsl:text>bankowskaq karta</xsl:text>
</xsl:when>
<xsl:when test="VR = '|r'">
<xsl:text>bankowskaq karta</xsl:text>
</xsl:when>
<xsl:when test="VR = 'p`'">
<xsl:text>beznali~nyj</xsl:text>
</xsl:when>
<xsl:when test="VR = 'W'">
<xsl:text>bankowskaq karta</xsl:text>
</xsl:when>
<xsl:when test="VR = 'Wn'">
<xsl:text>nali~nyj</xsl:text>
</xsl:when>
<xsl:when test="VR = 'n'">
<xsl:text>nali~nyj</xsl:text>
</xsl:when>
<xsl:otherwise>
???
</xsl:otherwise>
</xsl:choose>
<xsl:choose>
<xsl:when test="VR">
<xsl:text> (</xsl:text>
<xsl:value-of select="VR"/>
<xsl:text>)</xsl:text>
</xsl:when>
<xsl:otherwise>
net wida ras~eta
</xsl:otherwise>
</xsl:choose>
/CRLF
<xsl:if test="ELS">
<xsl:text>els </xsl:text>
<xsl:value-of select="ELS"/>
</xsl:if>
<xsl:if test="IG">
<xsl:text>, integrator</xsl:text>
</xsl:if>
<xsl:if test="R">/CRLF
<xsl:text>po ru~noj tehnologii</xsl:text>
</xsl:if>
/CRLF
<xsl:call-template name="DT">
<xsl:with-param name="NP" select="DEP/N1"/>
<xsl:with-param name="D" select="DEP/LD3"/>
<xsl:with-param name="T" select="DEP/LT1"/>
<xsl:with-param name="NAME" select="DEP/C1"/>
<xsl:with-param name="CODE" select="DEP/CC1"/>
</xsl:call-template>
<xsl:call-template name="DT3">
<xsl:with-param name="NP3" select="ARR/N5"/>
<xsl:with-param name="D3" select="ARR/LD6"/>
<xsl:with-param name="T3" select="ARR/LT4"/>
<xsl:with-param name="NAME3" select="ARR/C2"/>
<xsl:with-param name="CODE3" select="ARR/CC2"/>
</xsl:call-template>
<xsl:text>wagon </xsl:text>
<xsl:value-of select="CAR/VNP"/>
<xsl:text>, </xsl:text>
<xsl:value-of select="CARRIER/VB"/>
<xsl:text>, nomer uslugi </xsl:text>
<xsl:value-of select="INFOUS/NUS"/>
<xsl:text>, kod plateva </xsl:text>
<xsl:value-of select="INFOUS/PAY"/>
<xsl:text>, wid deqtelxnosti </xsl:text>
<xsl:value-of select="INFOUS/TYA"/>
/CRLF
/POS00/AHT<xsl:text>informaciq po usluge</xsl:text>
/POS26/AHT<xsl:text>pokupatelx</xsl:text>
/POS62/AHT<xsl:text>postaw}ik uslug</xsl:text>
/CRLF
<xsl:text>naimenowanie: </xsl:text>
/CRLF
<xsl:value-of select="INFOUS/NAMEUS"/>
/CRLF
<xsl:text>k proezdnomu dokumentu: </xsl:text>
/CRLF
<xsl:value-of select="NEB"/>
/CRLF
<xsl:text>stanciq predostawleniq</xsl:text>/CRLF
<xsl:text>uslugi: </xsl:text>
/CRLF
<xsl:value-of select="INFOUS/C"/>
<xsl:text> (</xsl:text>
<xsl:value-of select="INFOUS/CC"/>
<xsl:text>)</xsl:text>
/CRLF
<xsl:text>kol-wo uslug: </xsl:text>
<xsl:value-of select="INFOUS/R1"/>
/CRLF
<xsl:text>stoimostx: </xsl:text>
<xsl:value-of select="INFOUS/SE"/>
/POS16/RVTU
/POS26/AHT<xsl:value-of select="IP/PI/PS"/>
<xsl:text>/</xsl:text>
/CRLF
<xsl:choose>
<xsl:when test="IP/PI/IZ">
/POS26/AHT<xsl:value-of select="IP/PI/IZ"/>
</xsl:when>
<xsl:otherwise>
/POS26/AHT<xsl:value-of select="IP/PI/ORG_NAME"/>
</xsl:otherwise>
</xsl:choose>
/CRLF
/POS26/AHT<xsl:text>wid tarifa </xsl:text>
<xsl:choose>
<xsl:when test="INFOUS/VD">
<xsl:value-of select="INFOUS/VD"/>
</xsl:when>
<xsl:otherwise>
<xsl:value-of select="SHL/SL"/>
<xsl:if test="SHL/W1">
<xsl:text>/</xsl:text>
<xsl:value-of select="SHL/W1"/>
</xsl:if>
</xsl:otherwise>
</xsl:choose>
<xsl:if test="IP/DL">
<xsl:text>,</xsl:text>
/CRLF
/POS26/AHT<xsl:text>osnowanie: </xsl:text>
<xsl:value-of select="IP/DL"/>
</xsl:if>
/POS06/RVTU
/POS62/AHT<xsl:value-of select="INFOUS/NAMEPOST"/>
/POS14/RVTD
/CRLF
</xsl:template>
<xsl:template mode="KKAT02" match="SERVICE[KKAT='02']">
<xsl:text>dopolnitelxnaq usluga </xsl:text>
<xsl:value-of select="KRS"/>/CRLF
<xsl:choose>
<xsl:when test="K1 = 0"><xsl:text>oformlen </xsl:text></xsl:when>
<xsl:when test="K1 = 3"><xsl:text>annulirowan </xsl:text></xsl:when>
<xsl:when test="K1 = 4"><xsl:text>wozwra}en </xsl:text></xsl:when>
<xsl:otherwise>
<xsl:text>neizwestnoe zna~enie K1</xsl:text>
</xsl:otherwise>
</xsl:choose>
<xsl:value-of select="substring(SD, 9, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring(SD, 6, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring(SD, 1, 4)"/>
<xsl:text> w </xsl:text>
<xsl:value-of select="substring(TB, 1, 5)"/>
<xsl:text>, </xsl:text>
<xsl:value-of select="KN"/>
<xsl:text>, wid ras~eta </xsl:text>
<xsl:choose>
<xsl:when test="VR = '|f'">
<xsl:text>bankowskaq karta</xsl:text>
</xsl:when>
<xsl:when test="VR = '|r'">
<xsl:text>bankowskaq karta</xsl:text>
</xsl:when>
<xsl:when test="VR = 'p`'">
<xsl:text>beznali~nyj</xsl:text>
</xsl:when>
<xsl:when test="VR = 'W'">
<xsl:text>bankowskaq karta</xsl:text>
</xsl:when>
<xsl:when test="VR = 'Wn'">
<xsl:text>nali~nyj</xsl:text>
</xsl:when>
<xsl:when test="VR = 'n'">
<xsl:text>nali~nyj</xsl:text>
</xsl:when>
<xsl:otherwise>
???
</xsl:otherwise>
</xsl:choose>
<xsl:choose>
<xsl:when test="VR">
<xsl:text> (</xsl:text>
<xsl:value-of select="VR"/>
<xsl:text>)</xsl:text>
</xsl:when>
<xsl:otherwise>
net wida ras~eta
</xsl:otherwise>
</xsl:choose>
/CRLF
<xsl:if test="ELS">
<xsl:text>els </xsl:text>
<xsl:value-of select="ELS"/>
</xsl:if>
<xsl:if test="IG">
<xsl:text>, integrator</xsl:text>
</xsl:if>
<xsl:if test="R">/CRLF
<xsl:text>po ru~noj tehnologii</xsl:text>
</xsl:if>
/CRLF
<xsl:call-template name="DT">
<xsl:with-param name="NP" select="DEP/N1"/>
<xsl:with-param name="D" select="DEP/LD3"/>
<xsl:with-param name="T" select="DEP/LT1"/>
<xsl:with-param name="NAME" select="DEP/C1"/>
<xsl:with-param name="CODE" select="DEP/CC1"/>
</xsl:call-template>
<xsl:call-template name="DT3">
<xsl:with-param name="NP3" select="ARR/N5"/>
<xsl:with-param name="D3" select="ARR/LD6"/>
<xsl:with-param name="T3" select="ARR/LT4"/>
<xsl:with-param name="NAME3" select="ARR/C2"/>
<xsl:with-param name="CODE3" select="ARR/CC2"/>
</xsl:call-template>
<xsl:text>wagon </xsl:text>
<xsl:value-of select="CAR/VNP"/>
<xsl:text>, </xsl:text>
<xsl:value-of select="CARRIER/VB"/>
<xsl:text>, nomer uslugi </xsl:text>
<xsl:value-of select="INFOEX/NEX"/>
<xsl:text>, kod plateva </xsl:text>
<xsl:value-of select="INFOEX/PAY"/>
<xsl:text>, wid deqtelxnosti </xsl:text>
<xsl:value-of select="INFOEX/TYA"/>
/CRLF
/POS00/AHT<xsl:text>informaciq po usluge</xsl:text>
/POS26/AHT<xsl:text>pokupatelx</xsl:text>
/POS62/AHT<xsl:text>postaw}ik uslug</xsl:text>/CRLF
<xsl:text>naimenowanie: </xsl:text>/CRLF
<xsl:value-of select="substring(INFOEX/NAMEEX, 1, 25)"/>
/CRLF
<xsl:text>k proezdnomu dokumentu:</xsl:text>/CRLF
<xsl:value-of select="/NEB"/>
/CRLF
<xsl:text>data prowedeniq |kskursii: </xsl:text>/CRLF
<xsl:value-of select="substring(INFOEX/D3, 9, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring(INFOEX/D3, 6, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring(INFOEX/D3, 1, 4)"/>
/CRLF
<xsl:text>wremq na~ala |kskursii: </xsl:text>
<xsl:value-of select="substring(INFOEX/T1, 1, 5)"/>/CRLF
<xsl:text>wremq okon~aniq |kskursii: </xsl:text>
<xsl:value-of select="substring(INFOEX/T4, 1, 5)"/>/CRLF
<xsl:text>stanciq predostawleniq uslugi: </xsl:text>/CRLF
<xsl:value-of select="INFOEX/C"/>
<xsl:text> (</xsl:text>
<xsl:value-of select="INFOEX/CC"/>
<xsl:text>)</xsl:text>/CRLF
<xsl:text>kol-wo uslug: </xsl:text>
<xsl:value-of select="INFOEX/R1"/>
/CRLF
<xsl:text>stoimostx: </xsl:text>
<xsl:value-of select="INFOEX/SE"/>
/POS24/RVTU
/POS26/AHT<xsl:value-of select="IP/PI/PS"/>
<xsl:text>/</xsl:text>
/CRLF
<xsl:choose>
<xsl:when test="IP/PI/IZ">
/POS26/AHT<xsl:value-of select="IP/PI/IZ"/>
</xsl:when>
<xsl:otherwise>
/POS26/AHT<xsl:value-of select="IP/PI/ORG_NAME"/>
</xsl:otherwise>
</xsl:choose>
/CRLF
/POS26/AHT<xsl:text>wid tarifa </xsl:text>
<xsl:choose>
<xsl:when test="SHL/VD">
<xsl:value-of select="SHL/VD"/>
</xsl:when>
<xsl:otherwise>
<xsl:value-of select="SHL/SL"/>
<xsl:if test="SHL/W1">
<xsl:text>/</xsl:text>
<xsl:value-of select="SHL/W1"/>
</xsl:if>
</xsl:otherwise>
</xsl:choose>
<xsl:text>, </xsl:text>
<xsl:if test="SHL/DL">
/CTRL
/POS26/AHT<xsl:text>osnowanie: </xsl:text>
<xsl:value-of select="/SHL/DL"/>
</xsl:if>
/POS10/RVTU
/POS62/AHT<xsl:value-of select="INFOEX/NAMEPOST"/>
/POS14/RVTD
/CRLF
</xsl:template>
<xsl:template name="STATUS">
<xsl:param name="K"/>
<xsl:param name="OO1"/>
<xsl:param name="DETS"/>
<xsl:param name="GRUP"/>
<xsl:param name="ECP"/>
<xsl:param name="POEZDA"/>
<xsl:param name="HEIGHT"/>
/POS64/AHT<xsl:choose>
<xsl:when test="not ($K)">
<xsl:text>net statusa</xsl:text>/CRLF
/POS64/AHT<xsl:text>dokumenta </xsl:text>
</xsl:when>
<xsl:when test="$K = '0'">
<xsl:text>prodan</xsl:text>
</xsl:when>
<xsl:when test="$K = '3'">
<xsl:text>annulirowan</xsl:text>
</xsl:when>
<xsl:when test="$K = '4'">
<xsl:text>wozwra}en</xsl:text>
</xsl:when>
<xsl:when test="$K = '6'">
<xsl:text>dublikat ili</xsl:text>/CRLF
/POS64/AHT<xsl:text>bumavnaq kopiq</xsl:text>
</xsl:when>
<xsl:when test="$K = '8'">
<xsl:text>prerywanie</xsl:text>/CRLF
/POS64/AHT<xsl:text>poezdki</xsl:text>
</xsl:when>
<xsl:when test="$K = '9'">
<xsl:text>sel w poezd</xsl:text>
</xsl:when>
<xsl:otherwise>
<xsl:text>neizwestnyj</xsl:text>/CRLF
/POS64/AHT<xsl:text>status</xsl:text>/CRLF
/POS64/AHT<xsl:text>dokumenta: "</xsl:text><xsl:value-of select="$K"/><xsl:text>"</xsl:text>
</xsl:otherwise>
</xsl:choose>/CRLF
<xsl:if test="$OO1">
/POS64/AHT<xsl:text>oplata w poezde</xsl:text>/CRLF
/POS64/AHT<xsl:text>ili kasse</xsl:text>/CRLF
/POS64/AHT<xsl:text>ne pozdnee</xsl:text>/CRLF
/POS64/AHT<xsl:value-of select="substring(DY, 9, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring(DY, 6, 2)"/>
<xsl:text>.</xsl:text>
<xsl:value-of select="substring(DY, 1, 4)"/>/CRLF
</xsl:if>
<xsl:if test="$DETS">
/POS64/AHT<xsl:text>detskaq gruppa</xsl:text>/CRLF
</xsl:if>
<xsl:if test="$GRUP">
/POS64/AHT<xsl:text>gruppowoj</xsl:text>/CRLF
</xsl:if>
<xsl:if test="$ECP">
/POS64/AHT<xsl:text>oplata</xsl:text>/CRLF
/POS64/AHT<xsl:text>podtwervdena</xsl:text>/CRLF
</xsl:if>
<xsl:if test="$POEZDA">
/POS63/AHT<xsl:text>oformlen w poezde</xsl:text>/CRLF
</xsl:if>
/CRLF
<xsl:variable name="CK">
<xsl:choose>
<xsl:when test="not ($K)">
2
</xsl:when>
<xsl:when test="$K = '0'">
1
</xsl:when>
<xsl:when test="$K = '3'">
1
</xsl:when>
<xsl:when test="$K = '4'">
1
</xsl:when>
<xsl:when test="$K = '6'">
2
</xsl:when>
<xsl:when test="$K = '8'">
2
</xsl:when>
<xsl:when test="$K = '9'">
1
</xsl:when>
<xsl:otherwise>
3
</xsl:otherwise>
</xsl:choose>
</xsl:variable>
<xsl:variable name="CK2" select="$CK + count($OO1)*2 + count($DETS) + count($GRUP) + count($ECP) + 1"/>
<xsl:if test="$CK2 &lt; $HEIGHT">
/POS
<xsl:call-template name="TOST">
<xsl:with-param name="VAL" select="concat('0', ($HEIGHT - $CK2)*2)"/>
</xsl:call-template>
/RVTD
</xsl:if>
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
<xsl:when test="RESP/@TYPE ='SPR_ORDERSTATE'">
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
