<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0" xmlns:x="http://www.yandex.ru/xscript">
  <xsl:import href="x-tests.xsl"/>
  <xsl:template name="test-method">
    <xsl:value-of select="x:urldecode('cp1251', .)"/>
  </xsl:template>
</xsl:stylesheet>
