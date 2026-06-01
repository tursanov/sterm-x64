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
<xsl:template name="PART1">
<xsl:param name="MODE"/>
<xsl:choose>
<xsl:when test="KN">
<xsl:choose>
<xsl:when test="$MODE='D'">/POS00/AHT</xsl:when>
<xsl:otherwise>/CPI20/NF/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:text>smennyj ot~et za terminal </xsl:text>
<xsl:value-of select="KN"/>
</xsl:when>
<xsl:otherwise>
<xsl:choose>
<xsl:when test="$MODE='D'">/POS00/AHT</xsl:when>
<xsl:otherwise>/CPI20/NF/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:text>smennyj ot~et</xsl:text>
</xsl:otherwise>
</xsl:choose>
/CRLF
<xsl:choose>
<xsl:when test="$MODE='D'">/POS00/AHT</xsl:when>
<xsl:otherwise>/CPI20/NF/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:text>wid ot~eta: </xsl:text>
<xsl:value-of select="TYPE_REP"/>
<xsl:text>, nomer ot~eta </xsl:text>
<xsl:value-of select="NUM_REP"/>
/CRLF
<xsl:choose>
<xsl:when test="$MODE='D'">/POS00/AHT</xsl:when>
<xsl:otherwise>/CPI20/NF/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:text>kassir: </xsl:text>
<xsl:value-of select="IZ"/>
/CRLF
<xsl:choose>
<xsl:when test="$MODE='D'">/POS00/AHT</xsl:when>
<xsl:otherwise>/CPI20/NF/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:text>sertifikat kassira </xsl:text>
<xsl:value-of select="CERT"/>
/CRLF
</xsl:template>
<xsl:template name="PART2">
<xsl:param name="MODE"/>
<xsl:choose>
<xsl:when test="$MODE='D'">/POS00/AHT</xsl:when>
<xsl:otherwise>/CPI20/NF/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:text>zapas blankow </xsl:text>
<xsl:choose>
<xsl:when test="STOCK and STOK">
<xsl:text>oba</xsl:text>
</xsl:when>
<xsl:when test="STOCK">
<xsl:value-of select="STOCK"/>
</xsl:when>
<xsl:when test="STOK">
<xsl:value-of select="STOK"/>
</xsl:when>
<xsl:otherwise>net</xsl:otherwise>
</xsl:choose>
<xsl:choose>
<xsl:when test="DOC_NUM/XN or DOC_NUM/XK or DOC_NUM/XMN or DOC_NUM/XMK or DOC_NUM/ZN or DOC_NUM/ZK or DOC_NUM/QN or DOC_NUM/QK or DOC_NUM/XRT or DOC_NUM/LD">
<xsl:text>, nomera izrashodowannyh blankow:</xsl:text>
/CRLF
<xsl:choose>
<xsl:when test="$MODE='D'">/POS00/AHT</xsl:when>
<xsl:otherwise>/CPI12/NF/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:if test="DOC_NUM/XN or DOC_NUM/XK">
<xsl:text>X</xsl:text>
/POS09/AHT<xsl:value-of select="DOC_NUM/XN"/>
/POS22/AHT<xsl:value-of select="DOC_NUM/XK"/>
/CRLF
</xsl:if>
<xsl:if test="DOC_NUM/XMN or DOC_NUM/XMK">
<xsl:text>XM</xsl:text>
/POS09/AHT<xsl:value-of select="DOC_NUM/XMN"/>
/POS22/AHT<xsl:value-of select="DOC_NUM/XMK"/>/CRLF
</xsl:if>
<xsl:if test="DOC_NUM/ZN or DOC_NUM/ZK">
<xsl:text>Z</xsl:text>
/POS09/AHT<xsl:value-of select="DOC_NUM/ZN"/>
/POS22/AHT<xsl:value-of select="DOC_NUM/ZK"/>
/CRLF
</xsl:if>
<xsl:if test="DOC_NUM/QN or DOC_NUM/QK">
<xsl:text>Q</xsl:text>
/POS09/AHT<xsl:value-of select="DOC_NUM/QN"/>
/POS22/AHT<xsl:value-of select="DOC_NUM/QK"/>
/CRLF
</xsl:if>
<xsl:if test="DOC_NUM/XRT">
<xsl:text>~l</xsl:text>
/POS09/AHT<xsl:text>--------</xsl:text>
/POS22/AHT<xsl:value-of select="DOC_NUM/XRT"/>
/CRLF
</xsl:if>
<xsl:if test="DOC_NUM/LD">
<xsl:text>|-np</xsl:text>
/POS09/AHT<xsl:text>--------</xsl:text>
/POS22/AHT<xsl:value-of select="DOC_NUM/LD"/>
/CRLF
</xsl:if>
</xsl:when>
<xsl:otherwise>
/CRLF
</xsl:otherwise>
</xsl:choose>
/CRLF
<xsl:choose>
<xsl:when test="$MODE='D'">/POS00/AHT</xsl:when>
<xsl:otherwise>/CPI20/NF/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:text>wid</xsl:text>
/POS07/AHT<xsl:text>oform</xsl:text>
/POS13/AHT<xsl:text>oform</xsl:text>
/POS19/AHT<xsl:text>poga{</xsl:text>
/POS25/AHT<xsl:text>poga{</xsl:text>
/POS31/AHT<xsl:text>wozwr</xsl:text>
/POS37/AHT<xsl:text>prod</xsl:text>
/POS42/AHT<xsl:text>wozwr</xsl:text>
/CRLF
/POS00/AHT<xsl:text>dok-ta</xsl:text>
/POS07/AHT<xsl:text>dok-w</xsl:text>
/POS13/AHT<xsl:text>na ~l</xsl:text>
/POS19/AHT<xsl:text>dok-w</xsl:text>
/POS25/AHT<xsl:text>na ~l</xsl:text>
/POS31/AHT<xsl:text>dok-w</xsl:text>
/POS37/AHT<xsl:text>mest</xsl:text>
/POS42/AHT<xsl:text>mest</xsl:text>
/CRLF
/POS00/AHT<xsl:text>X</xsl:text>
/POS07/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C403, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C403"/></xsl:otherwise></xsl:choose>
/POS13/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C463, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C463"/></xsl:otherwise></xsl:choose>
/POS19/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C404, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C404"/></xsl:otherwise></xsl:choose>
/POS25/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C464, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C464"/></xsl:otherwise></xsl:choose>
/POS31/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C405, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C405"/></xsl:otherwise></xsl:choose>
/POS37/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C406, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C406"/></xsl:otherwise></xsl:choose>
/POS42/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C407, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C407"/></xsl:otherwise></xsl:choose>
/CRLF
/POS00/AHT<xsl:text>XM</xsl:text>
/POS07/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C426, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C426"/></xsl:otherwise></xsl:choose>
/POS13/AHT<xsl:text>-</xsl:text>
/POS19/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C427, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C427"/></xsl:otherwise></xsl:choose>
/POS25/AHT<xsl:text>-</xsl:text>
/POS31/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C431, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C431"/></xsl:otherwise></xsl:choose>
/POS37/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C430, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C430"/></xsl:otherwise></xsl:choose>
/POS42/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C434, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C434"/></xsl:otherwise></xsl:choose>
/CRLF
/POS00/AHT<xsl:text>Q</xsl:text>
/POS07/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C410, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C410"/></xsl:otherwise></xsl:choose>
/POS13/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C455, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C455"/></xsl:otherwise></xsl:choose>
/POS19/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C412, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C412"/></xsl:otherwise></xsl:choose>
/POS25/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C456, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C456"/></xsl:otherwise></xsl:choose>
/POS31/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C411, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C411"/></xsl:otherwise></xsl:choose>
/POS37/AHT<xsl:text>-</xsl:text>
/POS42/AHT<xsl:text>-</xsl:text>
/CRLF
/POS00/AHT<xsl:text>Z</xsl:text>
/POS07/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C415, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C415"/></xsl:otherwise></xsl:choose>
/POS13/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C401, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C401"/></xsl:otherwise></xsl:choose>
/POS19/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C416, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C416"/></xsl:otherwise></xsl:choose>
/POS25/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C402, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C402"/></xsl:otherwise></xsl:choose>
/POS31/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C420, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C420"/></xsl:otherwise></xsl:choose>
/POS37/AHT<xsl:text>-</xsl:text>
/POS42/AHT<xsl:text>-</xsl:text>
/CRLF
/POS00/AHT<xsl:text>Y</xsl:text>
/POS07/AHT<xsl:text>-</xsl:text>
/POS13/AHT<xsl:text>-</xsl:text>
/POS19/AHT<xsl:text>-</xsl:text>
/POS25/AHT<xsl:text>-</xsl:text>
/POS31/AHT<xsl:text>-</xsl:text>
/POS37/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C462, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C462"/></xsl:otherwise></xsl:choose>
/POS42/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C408, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C408"/></xsl:otherwise></xsl:choose>
/CRLF
/POS00/AHT<xsl:text>C</xsl:text>
/POS07/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C414, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C414"/></xsl:otherwise></xsl:choose>
/POS13/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C424, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C424"/></xsl:otherwise></xsl:choose>
/POS19/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C425, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C425"/></xsl:otherwise></xsl:choose>
/POS25/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C469, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C469"/></xsl:otherwise></xsl:choose>
/POS31/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C470, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C470"/></xsl:otherwise></xsl:choose>
/POS37/AHT<xsl:text>-</xsl:text>
/POS42/AHT<xsl:text>-</xsl:text>
/CRLF
<xsl:choose>
<xsl:when test="$MODE='D'">/POS00/AHT</xsl:when>
<xsl:otherwise>/CPI15/NF/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:text>wsego: </xsl:text>
<xsl:choose><xsl:when test="translate(INFO_COU/C439, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C439"/></xsl:otherwise></xsl:choose>
<xsl:text>, wd na ub: </xsl:text>
<xsl:choose><xsl:when test="translate(INFO_COU/C446, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C446"/></xsl:otherwise></xsl:choose>
<xsl:text>, obxem: </xsl:text>
<xsl:choose><xsl:when test="translate(INFO_COU/C445, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C445"/></xsl:otherwise></xsl:choose>
/CRLF
<xsl:text>talony: </xsl:text>
<xsl:choose><xsl:when test="translate(INFO_COU/C443, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C443"/></xsl:otherwise></xsl:choose>
<xsl:text>, sprawki: </xsl:text>
<xsl:choose><xsl:when test="translate(INFO_COU/KOL_SPR, ' ', '') = ''"><xsl:text>0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/KOL_SPR"/></xsl:otherwise></xsl:choose>
/CRLF
/CRLF
<xsl:choose>
<xsl:when test="$MODE='D'">/POS00/AHT</xsl:when>
<xsl:otherwise>/CPI20/NF/POS00/AHT</xsl:otherwise>
</xsl:choose>
/POS00/AHT<xsl:text>wid</xsl:text>
/POS12/AHT<xsl:text>tarif</xsl:text>
/POS27/AHT<xsl:text>tarif</xsl:text>
<xsl:if test="INFO_COU/C418">
/POS42/AHT<xsl:text>tarif</xsl:text>
</xsl:if>
/CRLF
/POS00/AHT<xsl:text>dokumenta</xsl:text>
/POS12/AHT<xsl:text>nali~nye</xsl:text>
/POS27/AHT<xsl:text>beznali~nye</xsl:text>
<xsl:if test="INFO_COU/C418">
/POS42/AHT<xsl:text>centralizowannyj</xsl:text>
</xsl:if>
/CRLF
/POS00/AHT<xsl:text>X+Q</xsl:text>
/POS12/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C435, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C435"/></xsl:otherwise></xsl:choose>
/POS27/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C436, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C436"/></xsl:otherwise></xsl:choose>
<xsl:if test="INFO_COU/C418">
/POS42/AHT<xsl:text>0.0</xsl:text>
</xsl:if>
/CRLF
/POS00/AHT<xsl:text>XM</xsl:text>
/POS12/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C428, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C428"/></xsl:otherwise></xsl:choose>
/POS27/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C429, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C429"/></xsl:otherwise></xsl:choose>
<xsl:if test="INFO_COU/C418">
/POS42/AHT<xsl:text>0.0</xsl:text>
</xsl:if>
/CRLF
/POS00/AHT<xsl:text>Z</xsl:text>
/POS12/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C417, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C417"/></xsl:otherwise></xsl:choose>
/POS27/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C419, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C419"/></xsl:otherwise></xsl:choose>
<xsl:if test="INFO_COU/C418">
/POS42/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C418, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C418"/></xsl:otherwise></xsl:choose>
</xsl:if>
/CRLF
/POS00/AHT<xsl:text>Y</xsl:text>
/POS12/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C409, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C409"/></xsl:otherwise></xsl:choose>
/POS27/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C413, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C413"/></xsl:otherwise></xsl:choose>
<xsl:if test="INFO_COU/C418">
/POS42/AHT<xsl:text>0.0</xsl:text>
</xsl:if>
/CRLF
/POS00/AHT<xsl:text>itogo</xsl:text>
/POS12/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C451, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C451"/></xsl:otherwise></xsl:choose>
/POS27/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C452, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C452"/></xsl:otherwise></xsl:choose>
<xsl:if test="INFO_COU/C418">
/POS42/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C418, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C418"/></xsl:otherwise></xsl:choose>
</xsl:if>
/CRLF
/POS00/AHT<xsl:text>osn</xsl:text>
/POS12/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C447, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C447"/></xsl:otherwise></xsl:choose>
/POS27/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C448, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C448"/></xsl:otherwise></xsl:choose>
<xsl:if test="INFO_COU/C418">
/POS42/AHT<xsl:text>0.0</xsl:text>
</xsl:if>
/CRLF
/POS00/AHT<xsl:text>pwd</xsl:text>
/POS12/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C449, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C449"/></xsl:otherwise></xsl:choose>
/POS27/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C450, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C450"/></xsl:otherwise></xsl:choose>
<xsl:if test="INFO_COU/C418">
/POS42/AHT<xsl:text>0.0</xsl:text>
</xsl:if>
/CRLF
/POS00/AHT<xsl:text>wsego</xsl:text>
/POS12/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C467, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C467"/></xsl:otherwise></xsl:choose>
/POS27/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C468, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C468"/></xsl:otherwise></xsl:choose>
<xsl:if test="INFO_COU/C418">
/POS42/AHT<xsl:text>0.0</xsl:text>
</xsl:if>
/CRLF
/CRLF
/POS00/AHT<xsl:text>wid</xsl:text>
/POS12/AHT<xsl:text>sbory</xsl:text>
/POS27/AHT<xsl:text>sbory</xsl:text>
<xsl:if test="INFO_COU/C423">
/POS42/AHT<xsl:text>sbory</xsl:text>
</xsl:if>
/CRLF
/POS00/AHT<xsl:text>dokumenta</xsl:text>
/POS12/AHT<xsl:text>nali~nye</xsl:text>
/POS27/AHT<xsl:text>beznali~nye</xsl:text>
<xsl:if test="INFO_COU/C423">
/POS42/AHT<xsl:text>centralizowannye</xsl:text>
</xsl:if>
/CRLF
/POS00/AHT<xsl:text>X+Q</xsl:text>
/POS12/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C438, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C438"/></xsl:otherwise></xsl:choose>
/POS27/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C440, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C440"/></xsl:otherwise></xsl:choose>
<xsl:if test="INFO_COU/C423">
/POS42/AHT<xsl:text>0.0</xsl:text>
</xsl:if>
/CRLF
/POS00/AHT<xsl:text>XM</xsl:text>
/POS12/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C432, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C432"/></xsl:otherwise></xsl:choose>
/POS27/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C433, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C433"/></xsl:otherwise></xsl:choose>
<xsl:if test="INFO_COU/C423">
/POS42/AHT<xsl:text>0.0</xsl:text>
</xsl:if>
/CRLF
/POS00/AHT<xsl:text>Z</xsl:text>
/POS12/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C421, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C421"/></xsl:otherwise></xsl:choose>
/POS27/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C422, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C422"/></xsl:otherwise></xsl:choose>
<xsl:if test="INFO_COU/C423">
/POS42/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C423, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C423"/></xsl:otherwise></xsl:choose>
</xsl:if>
/CRLF
/POS00/AHT<xsl:text>Y</xsl:text>
/POS12/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C459, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C459"/></xsl:otherwise></xsl:choose>
/POS27/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C461, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C461"/></xsl:otherwise></xsl:choose>
<xsl:if test="INFO_COU/C423">
/POS42/AHT<xsl:text>0.0</xsl:text>
</xsl:if>
/CRLF
/POS00/AHT<xsl:text>C</xsl:text>
/POS12/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C471, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C471"/></xsl:otherwise></xsl:choose>
/POS27/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C472, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C472"/></xsl:otherwise></xsl:choose>
<xsl:if test="INFO_COU/C423">
/POS42/AHT<xsl:text>0.0</xsl:text>
</xsl:if>
/CRLF
/POS00/AHT<xsl:text>itogo</xsl:text>
/POS12/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C458, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C458"/></xsl:otherwise></xsl:choose>
/POS27/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C460, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C460"/></xsl:otherwise></xsl:choose>
<xsl:if test="INFO_COU/C423">
/POS42/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C423, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C423"/></xsl:otherwise></xsl:choose>
</xsl:if>
/CRLF
/CRLF
/POS00/AHT<xsl:text>woin</xsl:text>
/POS12/AHT<xsl:choose><xsl:when test="translate(INFO_COU/C437, ' ', '') = ''"><xsl:text>0.0</xsl:text></xsl:when><xsl:otherwise><xsl:value-of select="INFO_COU/C437"/></xsl:otherwise></xsl:choose>
/CRLF
</xsl:template>
<xsl:template name="NUM2POS">
<xsl:param name="NUM"/>
<xsl:variable name="POS" select="concat('00',$NUM)"/>
<xsl:value-of select="substring($POS,string-length($POS)-1)"/>
</xsl:template>
<xsl:template name="PUTCOL">
<xsl:param name="COLNUM"/>
<xsl:param name="COLCNT"/>
<xsl:param name="COLWD"/>
<xsl:param name="COLBASED"/>
<xsl:param name="COLWP"/>
<xsl:param name="COLBASEP"/>
<xsl:param name="VAL"/>
<xsl:param name="MODE"/>
<xsl:choose>
<xsl:when test="$MODE='D'">
/POS
<xsl:call-template name="NUM2POS">
<xsl:with-param name="NUM" select="$COLBASED + (($COLNUM - 1) mod $COLCNT) * $COLWD"/>
</xsl:call-template>
/AHT
</xsl:when>
<xsl:otherwise>
/CPI20/NF
/POS
<xsl:call-template name="NUM2POS">
<xsl:with-param name="NUM" select="$COLBASEP + (($COLNUM - 1) mod $COLCNT) * $COLWP"/>
</xsl:call-template>
/AHT
</xsl:otherwise>
</xsl:choose>
<xsl:value-of select="$VAL"/>
</xsl:template>
<xsl:template name="PART3">
<xsl:param name="MODE"/>
<xsl:call-template name="PUTCOU">
<xsl:with-param name="CS" select="TICKET"/>
<xsl:with-param name="CT" select="'realizaciq proezdnyh dokumentow'"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
<xsl:call-template name="PUTCOU">
<xsl:with-param name="CS" select="BAGGAGE"/>
<xsl:with-param name="CT" select="'realizaciq perewozo~nyh dokumentow'"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
<xsl:call-template name="PUTCOU">
<xsl:with-param name="CS" select="TALON"/>
<xsl:with-param name="CT" select="'realizaciq sme{annyh i awtobusnyh perewozok'"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
<xsl:call-template name="PUTCOU">
<xsl:with-param name="CS" select="INSURANCE"/>
<xsl:with-param name="CT" select="'realizaciq strahowyh polisow'"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
<xsl:call-template name="PUTCOU">
<xsl:with-param name="CS" select="FOOD"/>
<xsl:with-param name="CT" select="'realizaciq dopolnitelxnogo pitaniq'"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
<xsl:call-template name="PUTCOU">
<xsl:with-param name="CS" select="CARD"/>
<xsl:with-param name="CT" select="'realizaciq dorovnyh kart i kart delowoj proezdnoj'"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
<xsl:call-template name="PUTCOU">
<xsl:with-param name="CS" select="SERVICE"/>
<xsl:with-param name="CT" select="'realizaciq dopolnitelxnyh uslug'"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
<xsl:call-template name="PUTCOU">
<xsl:with-param name="CS" select="SERVICENG"/>
<xsl:with-param name="CT" select="'realizaciq dopolnitelxnyh uslug asu &quot;|kspress np&quot;'"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
<xsl:call-template name="PUTCOU">
<xsl:with-param name="CS" select="OSN"/>
<xsl:with-param name="CT" select="'realizaciq po osn'"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
<xsl:call-template name="PUTCOU">
<xsl:with-param name="CS" select="PVD"/>
<xsl:with-param name="CT" select="'realizaciq po pwd'"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
<xsl:call-template name="PUTCOU">
<xsl:with-param name="CS" select="RT"/>
<xsl:with-param name="CT" select="'koli~estwo dokumentow, oformlennyh na ~ekowoj lente'"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
<xsl:call-template name="PUTCOU">
<xsl:with-param name="CS" select="TICKETR"/>
<xsl:with-param name="CT" select="'wwod ru~nyh dokumentow (dalxnee)'"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
<xsl:call-template name="PUTCOU">
<xsl:with-param name="CS" select="TICKETPR"/>
<xsl:with-param name="CT" select="'wwod ru~nyh dokumentow (prigorod)'"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
</xsl:template>
<xsl:template name="PUTCOU">
<xsl:param name="CS"/>
<xsl:param name="CT"/>
<xsl:param name="MODE"/>
<xsl:if test="$CS/COU">
<xsl:if test="$MODE='P'">
/NAK/ESC{/ESC!8003030/ESC#/POS02/AVT
</xsl:if>
<xsl:choose>
<xsl:when test="$MODE='D'">/POS00/AHT</xsl:when>
<xsl:otherwise>/CPI20/NF/POS00/AHT</xsl:otherwise>
</xsl:choose>
/CRLF
<xsl:value-of select="$CT"/>
/CRLF/CRLF
<xsl:for-each select="$CS/COU">
<xsl:if test="$MODE='P' and (position() != 1) and ((position() mod 4) = 1) and (((ceiling((position() - 1) div 4) * 5 ) mod 50) = 0)">
/NAK/ESC{/ESC!8003030/ESC#/POS02/AVT
</xsl:if>
<xsl:call-template name="PUTCOL">
<xsl:with-param name="COLNUM" select="position()"/>
<xsl:with-param name="COLCNT" select="4"/>
<xsl:with-param name="COLWD" select="15"/>
<xsl:with-param name="COLBASED" select="0"/>
<xsl:with-param name="COLWP" select="15"/>
<xsl:with-param name="COLBASEP" select="0"/>
<xsl:with-param name="VAL" select="substring(@C_NAME,1,13)"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
/CRLF
<xsl:call-template name="PUTCOL">
<xsl:with-param name="COLNUM" select="position()"/>
<xsl:with-param name="COLCNT" select="4"/>
<xsl:with-param name="COLWD" select="15"/>
<xsl:with-param name="COLBASED" select="0"/>
<xsl:with-param name="COLWP" select="15"/>
<xsl:with-param name="COLBASEP" select="0"/>
<xsl:with-param name="VAL" select="substring(@C_NAME,14,13)"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
/CRLF
<xsl:call-template name="PUTCOL">
<xsl:with-param name="COLNUM" select="position()"/>
<xsl:with-param name="COLCNT" select="4"/>
<xsl:with-param name="COLWD" select="15"/>
<xsl:with-param name="COLBASED" select="0"/>
<xsl:with-param name="COLWP" select="15"/>
<xsl:with-param name="COLBASEP" select="0"/>
<xsl:with-param name="VAL" select="substring(@C_NAME,27,13)"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
/CRLF
<xsl:call-template name="PUTCOL">
<xsl:with-param name="COLNUM" select="position()"/>
<xsl:with-param name="COLCNT" select="4"/>
<xsl:with-param name="COLWD" select="15"/>
<xsl:with-param name="COLBASED" select="6-ceiling(string-length(@C_NUM) div 2)"/>
<xsl:with-param name="COLWP" select="15"/>
<xsl:with-param name="COLBASEP" select="6-ceiling(string-length(@C_NUM) div 2)"/>
<xsl:with-param name="VAL" select="concat('(',@C_NUM,')')"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
/CRLF
<xsl:call-template name="PUTCOL">
<xsl:with-param name="COLNUM" select="position()"/>
<xsl:with-param name="COLCNT" select="4"/>
<xsl:with-param name="COLWD" select="15"/>
<xsl:with-param name="COLBASED" select="7-ceiling(string-length(@C_VALUE) div 2)"/>
<xsl:with-param name="COLWP" select="15"/>
<xsl:with-param name="COLBASEP" select="7-ceiling(string-length(@C_VALUE) div 2)"/>
<xsl:with-param name="VAL" select="@C_VALUE"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
/CRLF
<xsl:if test="((position() mod 4) != 0) and (position() != last())">
/POS
<xsl:choose>
<xsl:when test="$MODE='D'">10</xsl:when>
<xsl:otherwise>10</xsl:otherwise>
</xsl:choose>
/RVTU
</xsl:if>
</xsl:for-each>
</xsl:if>
</xsl:template>
<xsl:template name="PART4">
<xsl:param name="MODE"/>
<xsl:choose>
<xsl:when test="$MODE='D'">/POS00/AHT</xsl:when>
<xsl:otherwise>/CPI20/NF/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:text>stanciq </xsl:text>
<xsl:value-of select="C"/>
<xsl:text> (</xsl:text>
<xsl:value-of select="CC"/>
<xsl:text>)</xsl:text>
/CRLF
<xsl:choose>
<xsl:when test="$MODE='D'">/POS00/AHT</xsl:when>
<xsl:otherwise>/CPI20/NF/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:apply-templates select="ZG"/>
<xsl:text> agent </xsl:text><xsl:value-of select="NAME_AG"/>
/CRLF
</xsl:template>
<xsl:template mode="DP" match="RESP">
<xsl:param name="MODE"/>
<xsl:call-template name="PART1">
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
<xsl:call-template name="PART2">
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
<xsl:call-template name="PART3">
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
<xsl:if test="$MODE='P'">/NAK/ESC{/ESC!8003030/ESC#/POS02/AVT</xsl:if>
/CRLF
<xsl:call-template name="PART4">
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
</xsl:template>
<xsl:template mode="T8020" match="RESP">
<xsl:apply-templates mode="DP" select=".">
<xsl:with-param name="MODE" select="'D'"/>
</xsl:apply-templates>
</xsl:template>
<xsl:template mode="P" match="RESP">
<xsl:text disable-output-escaping="yes">/ESC{/ESC!8003030</xsl:text>
<xsl:text disable-output-escaping="yes">/ESC#/POS02/AVT</xsl:text>
<xsl:apply-templates mode="DP" select=".">
<xsl:with-param name="MODE" select="'P'"/>
</xsl:apply-templates>
<xsl:text disable-output-escaping="yes">/FF</xsl:text>
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
<xsl:when test="RESP/@TYPE ='REPORTK'">
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
