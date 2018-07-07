import React from 'react'
import Link from 'gatsby-link'

const Header = ({ siteTitle }) => (
  <div
    style={{
      background: '#ffc20e',
      
      // marginBottom: '1.45rem',
      opacity:'0.65',
      position:'absolute',
      width:'100%'
      // position:'relative'
    }}
  >
    <div
      style={{
        // margin: '0 auto',
        // maxWidth: 960,
        padding: '1.45rem 1.0875rem',
        display:'flex',
        justifyContent:'center',
        alignItems:'center',
        flexDirection:'column',
      }}
    >
      <h1 style={{ fontSize:'76px'  ,margin:'0px',color:'#000000',textDecoration:'none' }}> 
        {siteTitle}
      </h1>
      <br />
      <div 
        style={{ 
          display:'flex',
          alignItems:'center',
          flexDirection:'row'
        }}
      >
        <h3><Link to="/" style={{ color:'#000000',textDecoration:'none',margin:'20px' }}>HOME</Link></h3>
        <h3><Link to="/" style={{ color:'#000000',textDecoration:'none',margin:'20px' }}>ABOUT</Link></h3>
        <h3><Link to="/" style={{ color: '#000000', textDecoration: 'none',margin:'20px' }}>SPONSOR</Link></h3>
        <h3><Link to="/" style={{ color: '#000000', textDecoration: 'none',margin:'20px' }}>BLOG</Link></h3>

      </div>

    </div>
  </div>
)

export default Header
