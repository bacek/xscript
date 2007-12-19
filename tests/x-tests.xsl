<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  
  <xsl:output method="text" encoding="utf-8"/>
  
  <xsl:template match="/tests">
    
    <xsl:variable name="sources">
      <xsl:apply-templates select="/tests/test" mode="count"/>
    </xsl:variable>

    <xsl:variable name="successes">
      <xsl:apply-templates select="/tests/test"/>
    </xsl:variable>
	<result>
      <expected><xsl:value-of select="$sources"/></expected>
      <actual><xsl:value-of select="$successes"/></actual>
      <xsl:choose>
        <xsl:when test="string($sources) = string($successes)">
          <status>success</status>
        </xsl:when>
        <xsl:otherwise>
          <status>failed</status>
        </xsl:otherwise>
      </xsl:choose>
    </result>
  </xsl:template>
  
  <xsl:template match="/tests/test">
    <xsl:variable name="expected" select="result/text()"/>
    <xsl:variable name="result">
      <xsl:apply-templates select="source"/></xsl:variable>
    <xsl:choose>
      <xsl:when test="string($expected) = string($result)">
        <xsl:text>1</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>0</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="/tests/test" mode="count">
    <xsl:apply-templates select="source" mode="count"/>
  </xsl:template>

  <xsl:template match="/tests/test/source/text()">
    <xsl:call-template name="test-method"/>
  </xsl:template>

  <xsl:template match="/tests/test/source" mode="count">
    <xsl:text>1</xsl:text>
  </xsl:template>

</xsl:stylesheet>
