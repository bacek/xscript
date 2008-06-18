<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"
    xmlns:x="http://www.yandex.ru/xscript"
    extension-element-prefixes="x">
        
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
	<xsl:variable name="length" select="source/@length"/>
        <xsl:variable name="nodeset" select="x:wbr(source, $length)"/>
        <xsl:apply-templates select="result" mode="node_set">
	    <xsl:with-param name="NodeSet" select="$nodeset"/>
	</xsl:apply-templates>
    </xsl:template>

    <xsl:template match="/tests/test/result" mode="node_set">
	<xsl:param name="NodeSet"/>
	
	<xsl:apply-templates select="$NodeSet" mode="node_set">
	    <xsl:with-param name="NodePosition" select="position()"/>
	    <xsl:with-param name="NodeName" select="node-name"/>
	    <xsl:with-param name="NodeContent" select="node-content"/>
	</xsl:apply-templates>
	
    </xsl:template>

    <xsl:template match="node()" mode="node_set">
    	<xsl:param name="NodePosition"/>
	<xsl:param name="NodeName"/>
	<xsl:param name="NodeContent"/>

	<xsl:choose>
	    <xsl:when test="$NodePosition = position()">
	    
		<xsl:choose>
		    <xsl:when test="$NodeName = name()">
			<xsl:text>1</xsl:text>
		    </xsl:when>
		    <xsl:otherwise>
			<xsl:text>0</xsl:text>
		    </xsl:otherwise>
		</xsl:choose>
		
		<xsl:choose>
		    <xsl:when test="$NodeContent = .">
			<xsl:text>1</xsl:text>
		    </xsl:when>
		    <xsl:otherwise>
			<xsl:text>0</xsl:text>
		    </xsl:otherwise>
		</xsl:choose>
		
	    </xsl:when>
	</xsl:choose>

    </xsl:template>    

    <xsl:template match="/tests/test" mode="count">
        <xsl:apply-templates select="result" mode="count"/>
    </xsl:template>

    <xsl:template match="/tests/test/result" mode="count">
        <xsl:apply-templates select="node-name" mode="count"/>
	<xsl:apply-templates select="node-content" mode="count"/>
    </xsl:template>

    <xsl:template match="/tests/test/result/node-name" mode="count">
	<xsl:text>1</xsl:text>
    </xsl:template>
    
    <xsl:template match="/tests/test/result/node-content" mode="count">
	<xsl:text>1</xsl:text>
    </xsl:template>

</xsl:stylesheet>
