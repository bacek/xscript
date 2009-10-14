<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:math="http://exslt.org/math"
    extension-element-prefixes="math"
>

<xsl:template match="/page/response-time">
    <html>
        <head>
	    <title>Xscript5 response time statistics</title>
        </head>
        <body>
	    <h2>Xscript5 response time statistics</h2>
	    <div style="margin-left:5px">Collection time: <xsl:value-of select="@collect-time"/> sec.<br/><br/></div>
	    
	    <table border="1" cellpadding="5px">
	        <tr align="center">
		    <td>Status</td>
		    <td>Auth status</td>
		    <td>Request count</td>
		    <td>Average response</td>
		    <td>Min response</td>
		    <td>Max response</td>
		    <td>Total response</td>
		</tr>
		<xsl:apply-templates select="status"/>
	    </table>
	    <br/>
	    <div style="margin-left:5px">* Time data in microseconds</div>
	</body>
    </html>
</xsl:template>

<xsl:template match="status">
    <tr align="center">
        <td><xsl:value-of select="@code"/></td>
	<td>all</td>
        <td><xsl:value-of select="sum(child::point/@count)"/></td>
        <td><xsl:value-of select="floor(sum(child::point/@total) div sum(child::point/@count))"/></td>
        <td><xsl:value-of select="math:min(child::point/@min)"/></td>
        <td><xsl:value-of select="math:max(child::point/@max)"/></td>	
        <td><xsl:value-of select="sum(child::point/@total)"/></td>
    </tr>
    <xsl:apply-templates select="point"/>
</xsl:template>

<xsl:template match="point">
    <tr align="center">
        <td><xsl:value-of select="../@code"/></td>
	<td><xsl:value-of select="@auth-type"/></td>
	<td><xsl:value-of select="@count"/></td>        
	<td><xsl:value-of select="@avg"/></td>
        <td><xsl:value-of select="@min"/></td>
        <td><xsl:value-of select="@max"/></td>
        <td><xsl:value-of select="@total"/></td>
    </tr>
</xsl:template>

</xsl:stylesheet>