<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet  version="1.0" 
	xmlns:x="http://www.yandex.ru/xscript"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:import href="x-tests.xsl"/>
  
  <xsl:template name="test-method">
    <xsl:value-of select="x:urlencode('cp1251', .)"/>
  </xsl:template>

</xsl:stylesheet>
