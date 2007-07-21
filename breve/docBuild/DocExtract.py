
import inspect
import xml.dom.minidom

class DocExtract( object ):
	def Extract( self, c ):
		'''Extracts docs for a class'''

		methods = inspect.getmembers( c, inspect.ismethod )

		xmlimp = xml.dom.minidom.getDOMImplementation()

		xmldoc = xmlimp.createDocument( None, "class", None )

		rootnode = xmldoc.documentElement
		rootnode.setAttribute( 'name', c.__name__ )
		rootnode.setAttribute( 'parent', c.__base__.__name__ )

		if c.__doc__ != None:
			docstring = c.__doc__
		else:
			docstring = ''

		classdoc = xmldoc.createElement( 'classdocstring' )
		classdoc.appendChild( xmldoc.createTextNode( docstring ) )
		rootnode.appendChild( classdoc )

		for i in methods: 
			method = i[ 1 ]

			methodnode = xmldoc.createElement( 'method' )
			methodnode.setAttribute( 'name', i[ 0 ] )

			docnode = xmldoc.createElement( 'docstring' )

			if method.__doc__ != None:
				docstring = method.__doc__
			else:
				docstring = ''

			docnode.appendChild( xmldoc.createTextNode( docstring ) )

			methodnode.appendChild( docnode )

			arguments = inspect.getargspec( method )

			argsnode = xmldoc.createElement( 'arguments' )

			

			for n in xrange( len( arguments[ 0 ] ) ):
				argumentnode = xmldoc.createElement( 'argument' )
				argumentnode.setAttribute( 'name', arguments[ 0 ][ n ] )
				argsnode.appendChild( argumentnode )

			methodnode.appendChild( argsnode )

			rootnode.appendChild( methodnode )
			
		return xmldoc

	def Test( self ):
			'''Does some stuff'''

			pass


# extractor = DocExtract()
# extractor.Extract( DocExtract )
