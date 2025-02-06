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
<xsl:template mode="DP" match="RESP">
<xsl:param name="MODE"/>
<xsl:choose>
<xsl:when test="$MODE='P'">/CPI20/WF/POS01/AHT</xsl:when>
<xsl:otherwise>/POS26/AHT</xsl:otherwise>
</xsl:choose>
<xsl:text>|lektronnaq kwitanciq raznyh</xsl:text>/CRLF
<xsl:choose>
<xsl:when test="$MODE='P'">
/CPI20/WF/POS12/AHT</xsl:when>
<xsl:otherwise>/POS37/AHT</xsl:otherwise>
</xsl:choose>
<xsl:text>sborow</xsl:text>/CRLF
<xsl:choose>
<xsl:when test="$MODE='P'">/CPI12/NF/POS01/AHT</xsl:when>
<xsl:otherwise>/POS24/AHT</xsl:otherwise>
</xsl:choose>
<xsl:text>na okazanie dopolnitelxnyh uslug</xsl:text>/CRLF
<xsl:choose>
<xsl:when test="$MODE='P'">/CPI20/WF/POS08/AHT</xsl:when>
<xsl:otherwise>/POS33/AHT</xsl:otherwise>
</xsl:choose>
<xsl:choose>
<xsl:when test="SG1='001'">
<xsl:value-of select="KRS"/>
</xsl:when>
<xsl:when test="SG1='002'">
<xsl:value-of select="RET_KRS"/>
</xsl:when>
<xsl:when test="SG1='003'">
<xsl:value-of select="KRS"/>
</xsl:when>
</xsl:choose>
/CRLF
<xsl:choose>
<xsl:when test="$MODE='P'">/CPI20/WF/POS00/AHT</xsl:when>
<xsl:otherwise>/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:choose>
<xsl:when test="SG1='001'">
<xsl:text>oformlenie</xsl:text>
</xsl:when>
<xsl:when test="SG1='002'">
<xsl:text>wozwrat</xsl:text>
</xsl:when>
<xsl:when test="SG1='003'">
<xsl:text>ga{enie</xsl:text>
</xsl:when>
<xsl:otherwise>
<xsl:value-of select="SG1"/><xsl:text>: neovidannyj tip</xsl:text>
</xsl:otherwise>
</xsl:choose>
<xsl:text> uslugi</xsl:text>
<xsl:if test="SG1='002'">
<xsl:text> </xsl:text><xsl:value-of select="KRS"/>
</xsl:if>
/CRLF
<xsl:choose>
<xsl:when test="KKAT=1">
<xsl:call-template name="TUS">
<xsl:with-param name="NAME" select="INFOUS/NAMEUS"/>
<xsl:with-param name="NUM" select="INFOUS/NUS"/>
<xsl:with-param name="LUS" select="INFOUS"/>
<xsl:with-param name="SET" select="false()"/>
<xsl:with-param name="R1" select="INFOUS/R1"/>
<xsl:with-param name="PE" select="INFOUS/PE"/>
<xsl:with-param name="SE" select="INFOUS/SE"/>
<xsl:with-param name="STV" select="INFOUS/STV"/>
<xsl:with-param name="NDS" select="INFOUS/NDS"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
</xsl:when>
<xsl:when test="KKAT=2">
<xsl:call-template name="TUS">
<xsl:with-param name="NAME" select="INFOEX/NAMEEX"/>
<xsl:with-param name="NUM" select="INFOEX/NEX"/>
<xsl:with-param name="LUS" select="INFOEX"/>
<xsl:with-param name="SET" select="false()"/>
<xsl:with-param name="R1" select="INFOEX/R1"/>
<xsl:with-param name="PE" select="INFOEX/PE"/>
<xsl:with-param name="SE" select="INFOEX/SE"/>
<xsl:with-param name="STV" select="INFOEX/STV"/>
<xsl:with-param name="NDS" select="INFOEX/NDS"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
</xsl:when>
<xsl:when test="KKAT=3">
<xsl:call-template name="TUS">
<xsl:with-param name="NAME" select="INFOSET/NAMESET"/>
<xsl:with-param name="NUM" select="INFOSET/NSET"/>
<xsl:with-param name="LUS" select="INFOSET"/>
<xsl:with-param name="SET" select="true()"/>
<xsl:with-param name="R1" select="1"/>
<xsl:with-param name="PE" select="INFOSET/SE"/>
<xsl:with-param name="SE" select="INFOSET/SE"/>
<xsl:with-param name="STV" select="INFOSET/STV"/>
<xsl:with-param name="NDS" select="INFOSET/NDS"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
</xsl:when>
<xsl:when test="KKAT=5">
<xsl:call-template name="TUS">
<xsl:with-param name="NAME" select="INFOSKPL/NAMESKPL"/>
<xsl:with-param name="NUM" select="''"/>
<xsl:with-param name="LUS" select="INFOSKPL"/>
<xsl:with-param name="SET" select="false()"/>
<xsl:with-param name="R1" select="INFOSKPL/R1"/>
<xsl:with-param name="PE" select="INFOSKPL/PE"/>
<xsl:with-param name="SE" select="INFOSKPL/SE"/>
<xsl:with-param name="STV" select="INFOSKPL/STV"/>
<xsl:with-param name="NDS" select="INFOSKPL/NDS"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
</xsl:when>
<xsl:otherwise>
<xsl:value-of select="KKAT"/><xsl:text>: neovidannyj tip</xsl:text>
</xsl:otherwise>
</xsl:choose>
<xsl:apply-templates select="VR">
<xsl:with-param name="ELS" select="ELS"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:apply-templates>
/CRLF
<xsl:if test="ORG_NAME">
<xsl:choose>
<xsl:when test="$MODE='P'">/CPI12/NF/POS00/AHT</xsl:when>
<xsl:otherwise>/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:text>usluga predostawlqetsq dlq</xsl:text>/CRLF
<xsl:value-of select="ORG_NAME"/>/CRLF
</xsl:if>
<xsl:if test="NB">
<xsl:choose>
<xsl:when test="$MODE='P'">/CPI12/NF/POS00/AHT</xsl:when>
<xsl:otherwise>/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:text>usluga k proezdnomu dokumentu</xsl:text>/CRLF
<xsl:text>N</xsl:text><xsl:value-of select="NB"/>/CRLF
</xsl:if>
<xsl:if test="PI">
<xsl:choose>
<xsl:when test="$MODE='P'">/CPI12/NF/POS00/AHT</xsl:when>
<xsl:otherwise>/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:text>pokupatelx</xsl:text>/CRLF
<xsl:if test="PI/PS">
<xsl:choose>
<xsl:when test="not(PI/IZ)">
<xsl:choose>
<xsl:when test="$MODE='P'">/CPI15/WF/POS00/AHT</xsl:when>
<xsl:otherwise>/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:value-of select="PI/PS"/><xsl:text>/</xsl:text>
/CRLF
</xsl:when>
<xsl:otherwise>
<xsl:variable name="F40" select="concat(substring-before(PI/IZ,'='),'=')"/>
<xsl:variable name="IO" select="substring-after(PI/IZ,'=')"/>
<xsl:variable name="SL">
<xsl:choose>
<xsl:when test="$MODE='P'">59</xsl:when>
<xsl:otherwise>80</xsl:otherwise>
</xsl:choose>
</xsl:variable>
<xsl:choose>
<xsl:when test="not (string-length(concat(PI/PS,'/',PI/IZ)) &gt; $SL)">
<xsl:choose>
<xsl:when test="$MODE='P'">/CPI20/NF/POS00/AHT</xsl:when>
<xsl:otherwise>/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:value-of select="PI/PS"/>
<xsl:text>/</xsl:text>
<xsl:value-of select="PI/IZ"/>
</xsl:when>
<xsl:when test="not (string-length(concat(PI/PS,'/',$F40)) &gt; $SL)">
<xsl:choose>
<xsl:when test="$MODE='P'">/CPI20/NF/POS00/AHT</xsl:when>
<xsl:otherwise>/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:value-of select="PI/PS"/>
<xsl:text>/</xsl:text>
<xsl:value-of select="$F40"/>/CRLF
<xsl:value-of select="$IO"/>
</xsl:when>
<xsl:when test="not (string-length(PI/IZ) &gt; $SL)">
<xsl:choose>
<xsl:when test="$MODE='P'">/CPI20/NF/POS00/AHT</xsl:when>
<xsl:otherwise>/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:value-of select="PI/PS"/><xsl:text>/</xsl:text>/CRLF
<xsl:value-of select="PI/IZ"/>
</xsl:when>
<xsl:otherwise>
<xsl:choose>
<xsl:when test="$MODE='P'">/CPI15/NF/POS00/AHT</xsl:when>
<xsl:otherwise>/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:value-of select="PI/PS"/><xsl:text>/</xsl:text>/CRLF
<xsl:value-of select="$F40"/>/CRLF
<xsl:value-of select="$IO"/>
</xsl:otherwise>
</xsl:choose>
/CRLF
</xsl:otherwise>
</xsl:choose>
</xsl:if>
</xsl:if>
<xsl:if test="INFOEX/VD">
<xsl:call-template name="TARIF">
<xsl:with-param name="NAME" select="INFOEX/VD"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
/CRLF
</xsl:if>
<xsl:if test="INFOSET/VD">
<xsl:call-template name="TARIF">
<xsl:with-param name="NAME" select="INFOSET/VD"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
/CRLF
</xsl:if>
<xsl:choose>
<xsl:when test="$MODE='P'">/CPI12/NF/POS00/AHT</xsl:when>
<xsl:otherwise>/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:text>sluvebnaq informaciq</xsl:text>/CRLF
<xsl:choose>
<xsl:when test="$MODE='P'">/CPI20/NF/POS00/AHT</xsl:when>
<xsl:otherwise>/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:apply-templates select="ZG"/>
<xsl:if test="PNR">
<xsl:text> PNR </xsl:text><xsl:value-of select="PNR"/>
</xsl:if>
/CRLF
<xsl:if test="$MODE='P'">
/ESC/SUB02311008;014<xsl:value-of select="KRS"/>
</xsl:if>
</xsl:template>
<xsl:template name="TARIF">
<xsl:param name="NAME"/>
<xsl:param name="MODE"/>
<xsl:choose>
<xsl:when test="$MODE='P'">/CPI12/NF/POS00/AHT</xsl:when>
<xsl:otherwise>/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:text>tarif: </xsl:text>
<xsl:choose>
<xsl:when test="$NAME=1">
<xsl:text>polnyj</xsl:text>
</xsl:when>
<xsl:when test="$NAME=2">
<xsl:text>detskij</xsl:text>
</xsl:when>
<xsl:otherwise>
<xsl:text>?</xsl:text><xsl:value-of select="$NAME"/><xsl:text>?</xsl:text>
</xsl:otherwise>
</xsl:choose>
</xsl:template>
<xsl:template name="NUS">
<xsl:param name="NAME"/>
<xsl:param name="NUM"/>
<xsl:param name="US"/>
<xsl:param name="MODE"/>
<xsl:choose>
<xsl:when test="$MODE='P'">/CPI20/NF/POS00/AHT</xsl:when>
<xsl:otherwise>/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:variable name="RW">
<xsl:choose>
<xsl:when test="$MODE='P'">42</xsl:when>
<xsl:otherwise>63</xsl:otherwise>
</xsl:choose>
</xsl:variable>
<xsl:value-of select="substring($NAME,1,$RW)"/>
<xsl:variable name="BR">
<xsl:text>(</xsl:text>
<xsl:if test="string-length($NUM) &gt; 0"><xsl:value-of select="$NUM"/><xsl:text> </xsl:text>
</xsl:if>
<xsl:value-of select="$US/PAY"/><xsl:text> </xsl:text>
<xsl:choose>
<xsl:when test="$US/TYA=1">
<xsl:text>osn</xsl:text>
</xsl:when>
<xsl:when test="$US/TYA=2">
<xsl:text>pwd</xsl:text>
</xsl:when>
<xsl:otherwise>
<xsl:text>?</xsl:text><xsl:value-of select="substring($US/TYA,1,1)"/><xsl:text>?</xsl:text>
</xsl:otherwise>
</xsl:choose>
<xsl:text>)</xsl:text>
</xsl:variable>
<xsl:choose>
<xsl:when test="$MODE='P'">/POS43/AHT</xsl:when>
<xsl:otherwise>/POS64/AHT</xsl:otherwise>
</xsl:choose>
<xsl:value-of select="substring(concat('      ',$BR),7-(16-string-length($BR)))"/>
/CRLF
<xsl:if test="string-length($NAME) &gt; $RW">
<xsl:choose>
<xsl:when test="$MODE='P'">/CPI20/NF/POS00/AHT</xsl:when>
<xsl:otherwise>/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:value-of select="substring($NAME,$RW+1,$RW)"/>
/CRLF
</xsl:if>
<xsl:if test="string-length($NAME) &gt; 2 * $RW">
<xsl:choose>
<xsl:when test="$MODE='P'">/CPI20/NF/POS00/AHT</xsl:when>
<xsl:otherwise>/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:value-of select="substring($NAME,2 * $RW+1,$RW)"/>
/CRLF
</xsl:if>
</xsl:template>
<xsl:template name="POSTUS">
<xsl:param name="NAME"/>
<xsl:param name="INN"/>
<xsl:param name="MODE"/>
<xsl:choose>
<xsl:when test="$MODE='P'">/CPI20/WF/POS00/AHT</xsl:when>
<xsl:otherwise>/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:text>postaw}ik uslug</xsl:text>
/CRLF
<xsl:choose>
<xsl:when test="$MODE='P'">/CPI12/NF/POS00/AHT</xsl:when>
<xsl:otherwise>/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:value-of select="$NAME"/>
<xsl:choose>
<xsl:when test="$MODE='P'">/CPI12/NF/POS18/AHT</xsl:when>
<xsl:otherwise>/POS64/AHT</xsl:otherwise>
</xsl:choose>
<xsl:value-of select="substring(concat('  inn ',$INN),3-(12-string-length($INN)))"/>
/CRLF
</xsl:template>
<xsl:template name="PRICEUS">
<xsl:param name="CNT"/>
<xsl:param name="PRICE"/>
<xsl:param name="SUMUS"/>
<xsl:param name="STVUS"/>
<xsl:param name="NDSUS"/>
<xsl:param name="MODE"/>
<xsl:choose>
<xsl:when test="$MODE='P'">/CPI12/NF/POS18/AHT</xsl:when>
<xsl:otherwise>/POS64/AHT</xsl:otherwise>
</xsl:choose>
<xsl:variable name="PR" select="concat(number($CNT),'*',$PRICE,' rub')"/>
<xsl:value-of select="substring(concat('       ',$PR),8-(16-string-length($PR)))"/>
/CRLF
<xsl:choose>
<xsl:when test="$MODE='P'">/CPI15/WF/POS00/AHT</xsl:when>
<xsl:otherwise>/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:text>wsego:</xsl:text>
<xsl:choose>
<xsl:when test="$MODE='P'">/CPI15/WF/POS08/AHT</xsl:when>
<xsl:otherwise>/POS67/AHT</xsl:otherwise>
</xsl:choose>
<xsl:value-of select="substring(concat('      ',$SUMUS,' rub'),7-(9-string-length($SUMUS)))"/>
/CRLF
<xsl:choose>
<xsl:when test="$MODE='P'">/CPI20/WF/POS01/AHT</xsl:when>
<xsl:otherwise>/POS51/AHT</xsl:otherwise>
</xsl:choose>
<xsl:variable name="VNDS" select="concat('(w t.~. nds ',number($STVUS),'% ',$NDSUS,')')"/>
<xsl:value-of select="substring(concat('         ',$VNDS),10-(29-string-length($VNDS)))"/>
/CRLF
</xsl:template>
<xsl:template name="TUS">
<xsl:param name="NAME"/>
<xsl:param name="NUM"/>
<xsl:param name="LUS"/>
<xsl:param name="SET"/>
<xsl:param name="R1"/>
<xsl:param name="PE"/>
<xsl:param name="SE"/>
<xsl:param name="STV"/>
<xsl:param name="NDS"/>
<xsl:param name="MODE"/>
<xsl:call-template name="NUS">
<xsl:with-param name="NAME" select="$NAME"/>
<xsl:with-param name="NUM" select="$NUM"/>
<xsl:with-param name="US" select="$LUS"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
<xsl:choose>
<xsl:when test="$SET">
<xsl:choose>
<xsl:when test="(count($LUS/INFOUS)+count($LUS/INFOEX))=0">
<xsl:text>net uslug w nabore</xsl:text>/CRLF
</xsl:when>
<xsl:otherwise>
<xsl:for-each select="$LUS/INFOUS">
<xsl:if test="($MODE='P') and ((position() mod 6)=1)">/NAK/ESC{/ESC!8003030/ESC#/POS02/AVT</xsl:if>
<xsl:call-template name="NUS">
<xsl:with-param name="NAME" select="NAMEUS"/>
<xsl:with-param name="NUM" select="NUS"/>
<xsl:with-param name="US" select="."/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
<xsl:call-template name="POSTUS">
<xsl:with-param name="NAME" select="NAMEPOST"/>
<xsl:with-param name="INN" select="INNPOST"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
<xsl:call-template name="PRICEUS">
<xsl:with-param name="CNT" select="R1"/>
<xsl:with-param name="PRICE" select="PE"/>
<xsl:with-param name="SUMUS" select="SE"/>
<xsl:with-param name="STVUS" select="STV"/>
<xsl:with-param name="NDSUS" select="NDS"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
</xsl:for-each>
<xsl:for-each select="$LUS/INFOEX">
<xsl:if test="($MODE='P') and (((count($LUS/INFOUS) + position()) mod 6)=1)">/NAK/ESC{/ESC!8003030/ESC#/POS02/AVT</xsl:if>
<xsl:call-template name="NUS">
<xsl:with-param name="NAME" select="NAMEEX"/>
<xsl:with-param name="NUM" select="NEX"/>
<xsl:with-param name="US" select="."/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
<xsl:call-template name="POSTUS">
<xsl:with-param name="NAME" select="NAMEPOST"/>
<xsl:with-param name="INN" select="INNPOST"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
<xsl:call-template name="PRICEUS">
<xsl:with-param name="CNT" select="R1"/>
<xsl:with-param name="PRICE" select="PE"/>
<xsl:with-param name="SUMUS" select="SE"/>
<xsl:with-param name="STVUS" select="STV"/>
<xsl:with-param name="NDSUS" select="NDS"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
</xsl:for-each>
<xsl:if test="$MODE='P'">/NAK/ESC{/ESC!8003030/ESC#/POS02/AVT</xsl:if>
</xsl:otherwise>
</xsl:choose>
</xsl:when>
<xsl:otherwise>
<xsl:call-template name="POSTUS">
<xsl:with-param name="NAME" select="$LUS/NAMEPOST"/>
<xsl:with-param name="INN" select="$LUS/INNPOST"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
<xsl:call-template name="PRICEUS">
<xsl:with-param name="CNT" select="$R1"/>
<xsl:with-param name="PRICE" select="$PE"/>
<xsl:with-param name="SUMUS" select="$SE"/>
<xsl:with-param name="STVUS" select="$STV"/>
<xsl:with-param name="NDSUS" select="$NDS"/>
<xsl:with-param name="MODE" select="$MODE"/>
</xsl:call-template>
</xsl:otherwise>
</xsl:choose>
<xsl:choose>
<xsl:when test="$MODE='P'">/CPI15/WF/POS00/AHT</xsl:when>
<xsl:otherwise>/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:text>itogo:</xsl:text>
<xsl:choose>
<xsl:when test="$MODE='P'">/CPI15/WF/POS08/AHT</xsl:when>
<xsl:otherwise>/POS67/AHT</xsl:otherwise>
</xsl:choose>
<xsl:value-of select="substring(concat('      ',$SE,' rub'),7-(9-string-length($SE)))"/>
/CRLF
</xsl:template>
<xsl:template match="VR">
<xsl:param name="ELS"/>
<xsl:param name="MODE"/>
<xsl:choose>
<xsl:when test="$MODE='P'">/CPI15/NF/POS00/AHT</xsl:when>
<xsl:otherwise>/POS00/AHT</xsl:otherwise>
</xsl:choose>
<xsl:text>wid ras~eta </xsl:text>
<xsl:choose>
<xsl:when test="$MODE='P'">/POS16/AHT</xsl:when>
<xsl:otherwise>/POS54/AHT</xsl:otherwise>
</xsl:choose>
<xsl:variable name="V">
<xsl:choose>
<xsl:when test=".='sb'">
<xsl:text>sbp</xsl:text>
</xsl:when>
<xsl:when test="(.='|f') or (.='|r') or (.='W')">
<xsl:text>bankowskaq karta</xsl:text>
</xsl:when>
<xsl:when test="(.='n') or (.='Wn')">
<xsl:text>nali~nyj</xsl:text>
</xsl:when>
<xsl:when test=".='p`'">
<xsl:text>beznali~nyj</xsl:text>
<xsl:if test="$ELS">
<xsl:text> </xsl:text><xsl:value-of select="$ELS"/>
</xsl:if>
</xsl:when>
<xsl:otherwise>
<xsl:text>neovidannyj wid</xsl:text>
</xsl:otherwise>
</xsl:choose>
<xsl:text>(</xsl:text>
<xsl:value-of select="substring(.,1,2)"/>
<xsl:text>)</xsl:text>
</xsl:variable>
<xsl:value-of select="substring(concat('                   ',$V),20-(26-string-length($V)))"/>
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
<xsl:when test="RESP/@TYPE='SERVICE_BUY' or RESP/@TYPE='SERVICE_REJECT' or RESP/@TYPE='SERVICE_RETURN'">
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
