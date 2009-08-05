<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"
    xmlns:x="http://www.yandex.ru/xscript"
    xmlns:re="http://exslt.org/regular-expressions"
    extension-element-prefixes="x re">
    
   <xsl:template match="/">
     <result>
          <replace-test>
            <xsl:value-of select="re:replace('BASome textAB', 'A(.*)A', 'g', 'XXX $70000')"/>
          </replace-test>
     </result>
   </xsl:template>

</xsl:stylesheet>
