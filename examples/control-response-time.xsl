<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:math="http://exslt.org/math"
    extension-element-prefixes="math"
>

<xsl:variable name="collect-time" select="/page/response-time/@collect-time"/>
<xsl:variable name="days" select="floor($collect-time div 79800)"/>
<xsl:variable name="hours" select="floor(($collect-time - 79800*$days) div 3600)"/>
<xsl:variable name="minutes" select="floor(($collect-time - 79800*$days - 3600*$hours) div 60)"/>

<xsl:template match="/page/response-time">
    <html>
        <head>
	    <title>Xscript5 response time statistics</title>
        </head>
        <body>
	    <h2>Xscript5 response time statistics</h2>

	    <div style="margin-left:5px">Collection time: 
		<xsl:value-of select="$days"/> days
		<xsl:value-of select="$hours"/> hours 
		<xsl:value-of select="$minutes"/> minutes
		<br/><br/></div>
	    
	    <table border="1" cellpadding="5px">
	        <tr align="center">
		    <td>Status</td>
		    <td>Auth status</td>
		    <td>Count</td>
		    <td>Avg time</td>
		    <td>Min time</td>
		    <td>Max time</td>
<!--		    <td>Total response</td>  -->
		    <td>RPS</td>
		</tr>
		<xsl:if test="$collect-time > 0">
		    <xsl:apply-templates select="status"/>
		</xsl:if>
	    </table>
	    <br/>
	    <div style="margin-left:5px">* Time data in seconds</div>
	</body>
    </html>
</xsl:template>

<xsl:template match="status">
    <tr colspan="8"><td></td></tr>
    <tr align="center" style="font-weight:normal;background-color:#dddddd">
        <td><xsl:value-of select="@code"/></td>
	<td>all</td>
        <td><xsl:value-of select="sum(child::point/@count)"/></td>
	<xsl:variable name="sum" select="sum(child::point/@count)"/>
	<td>
	    <xsl:choose>
	        <xsl:when test="$sum = 0">
	            <xsl:value-of select="0"/>
	        </xsl:when>
		<xsl:otherwise>
		    <xsl:value-of select="format-number(sum(child::point/@total) div ($sum*1e6), '#.###')"/>
		</xsl:otherwise>
	    </xsl:choose>
        </td>
        <td><xsl:value-of select="format-number(math:min(child::point/@min) div 1e6, '#.###')"/></td>
        <td><xsl:value-of select="format-number(math:max(child::point/@max) div 1e6, '#.###')"/></td>	
<!--        <td><xsl:value-of select="sum(child::point/@total)"/></td>  -->
        <td><xsl:value-of select="format-number(sum(child::point/@count) div $collect-time, '#.###')"/></td>
    </tr>
 
    <xsl:apply-templates select="point"/>

</xsl:template>

<xsl:template match="point">
    <tr align="center">
        <td><xsl:value-of select="../@code"/></td>
	<td><xsl:value-of select="@auth-type"/></td>
	<td><xsl:value-of select="@count"/></td>        
	<td><xsl:value-of select="format-number(@avg div 1e6, '#.###')"/></td>
        <td><xsl:value-of select="format-number(@min div 1e6, '#.###')"/></td>
        <td><xsl:value-of select="format-number(@max div 1e6, '#.###')"/></td>
<!--        <td><xsl:value-of select="@total"/></td>   -->
	<td><xsl:value-of select="format-number(@count div $collect-time, '#.###')"/></td>
    </tr>
</xsl:template>

</xsl:stylesheet>